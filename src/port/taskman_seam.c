#include "nds_scene_harness_config.h"

#include <nds/nds_freeze_diagnostics.h>

extern u32 sySchedulerGetTicCount(void);
extern void sySchedulerSetTicCount(u32 tics);

#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
extern void ndsMNVSResultsRecordFrame(void);
extern void ndsSObjPreviewBeginFrame(void);
extern void ndsSObjPreviewEndFrame(void);
#endif

void ndsResetStartupDiagnostics(void)
{
    gNdsSceneBoundaryResult = 0;
    gNdsSceneBoundaryKind = 0;
    gNdsStartupTaskmanResult = 0;
    gNdsStartupTaskmanSceneKind = 0;
    gNdsStartupTaskmanDL0Size = 0;
    gNdsStartupTaskmanDL1Size = 0;
    gNdsStartupTaskmanControllerSet = 0;
    gNdsStartupFuncStartResult = 0;
    gNdsStartupSkipAllowWait = 0;
    gNdsStartupProceedOpening = 0;
    gNdsStartupGObjCreateCount = 0;
    gNdsStartupCameraCreateCount = 0;
    gNdsStartupRelocInitCount = 0;
    gNdsStartupSpriteCreateCount = 0;
    gNdsStartupFadeCreateCount = 0;
    gNdsStartupWallpaperParentValid = 0;
    gNdsStartupLogoPosX = 0;
    gNdsStartupLogoPosY = 0;
    gNdsStartupLogoFastcopyCleared = 0;
    gNdsStartupLogoRelocResult = 0;
    gNdsStartupLogoRelocSize = 0;
    gNdsStartupLogoRelocWordSwapCount = 0;
    gNdsStartupLogoRelocPointerFixupCount = 0;
    gNdsStartupLogoDrawResult = 0;
    gNdsStartupLogoDrawBlocker = NDS_STARTUP_LOGO_BLOCKER_NONE;
    gNdsStartupLogoDrawCallbackCount = 0;
    gNdsStartupLogoDrawUpdateCount = 0;
    gNdsStartupLogoDrawWidth = 0;
    gNdsStartupLogoDrawHeight = 0;
    gNdsStartupLogoDrawFormat = 0xffffffffu;
    gNdsStartupLogoDrawSize = 0xffffffffu;
    gNdsStartupLogoDrawBitmaps = 0;
    gNdsStartupLogoDrawPixels = 0;
    gNdsStartupLogoDrawGObjID = 0xffffffffu;
    gNdsStartupLogoDrawGObjObjKind = 0xffffffffu;
    gNdsStartupLogoDrawSObjAttr = 0xffffffffu;
    gNdsStartupLogoDrawTexshuf = 0;
    gNdsStartupLogoDrawTexshufSamples = 0;
    gNdsStartupLogoDrawVisibleSObjCount = 0;
    gNdsStartupActorFuncSet = 0;
    gNdsStartupWallpaperProcessKind = 0xffffffffu;
    gNdsStartupWallpaperProcessPriority = 0xffffffffu;
    gNdsStartupWallpaperDisplaySet = 0;
    gNdsStartupWallpaperCameraMaskLow = 0;
    gNdsStartupDefaultCameraColor = 0;
    gNdsTaskmanBridgeResult = 0;
    gNdsTaskmanContexts = 0;
    gNdsTaskmanTaskGfxNum = 0;
    gNdsTaskmanGraphicsHeapSize = 0;
    gNdsTaskmanRdpKind = 0;
    gNdsTaskmanRdpBufferSize = 0;
    gNdsTaskmanMallocCount = 0;
    gNdsStartupTaskmanMallocCount = 0;
    gNdsTaskmanGeneralHeapUsed = 0;
    gNdsTaskmanDLContextsValid = 0;
    gNdsTaskmanControllerAutoRead = 0;
    gNdsTaskmanSceneUpdateSet = 0;
    gNdsTaskmanSceneDrawSet = 0;
    gNdsTaskmanLightsSet = 0;
    gNdsTaskmanLoopReached = 0;
    gNdsTaskmanBoundedUpdateCount = 0;
    gNdsTaskmanPostUpdateSkip = 0;
    gNdsTaskmanGObjThreadSleeps = 0;
    gNdsTaskmanPostUpdateLogoPosX = 0;
    gNdsTaskmanPostUpdateLogoPosY = 0;
    gNdsTaskmanPostUpdateOpening = 0;
    gNdsTaskmanPostUpdateSceneKind = 0;
    gNdsTaskmanPostUpdateScenePrev = 0;
    gNdsTaskmanPostUpdateStatus = 0;
    gNdsTaskmanPostUpdateGObjCount = 0;
    gNdsTaskmanPostUpdateFadeCount = 0;
    gNdsTaskmanCleanupResult = 0;
    gNdsTaskmanCleanupQueuesEmpty = 0;
    gNdsTaskmanCleanupMode = 0;
    gNdsTaskmanReturnCount = 0;
    gNdsMemoryLedgerResult = 0;
    gNdsMemoryLedgerScene = 0;
    gNdsMemoryLedgerGeneration = 0;
    gNdsMemoryLedgerArenaCapacity = 0;
    gNdsMemoryLedgerArenaUsed = 0;
    gNdsMemoryLedgerArenaHighWater = 0;
    gNdsMemoryLedgerArenaHeadroom = 0;
    gNdsMemoryLedgerDLBytes = 0;
    gNdsMemoryLedgerGraphicsBytes = 0;
    gNdsMemoryLedgerRdpBytes = 0;
    gNdsMemoryLedgerFigatreeHeapSize = 0;
    gNdsMemoryLedgerRelocFiles = 0;
    gNdsMemoryLedgerRelocBytes = 0;
    gNdsMemoryLedgerRelocStageBytes = 0;
    gNdsMemoryLedgerRelocFighterBytes = 0;
    gNdsMemoryLedgerRelocInterfaceBytes = 0;
    gNdsMemoryLedgerRelocMenuBytes = 0;
    gNdsMemoryLedgerRelocOpeningBytes = 0;
    gNdsMemoryLedgerRelocOtherBytes = 0;
    gNdsMemoryLedgerRelocStaleFiles = 0;
    gNdsMemoryLedgerRelocStaleBytes = 0;
    gNdsMemoryLedgerEvictedFiles = 0;
    gNdsMemoryLedgerEvictedBytes = 0;
    ndsAudioAssetDiagnosticsReset();
    ndsAudioBgmDiagnosticsReset();
    gNdsOpeningRoomDispatchCount = 0;
    gNdsOpeningRoomStartResult = 0;
    gNdsOpeningRoomFuncStartResult = 0;
    gNdsOpeningRoomUpdateResult = 0;
    gNdsOpeningRoomTickCount = 0;
    gNdsOpeningMovieRoomHandoffResult = 0;
    gNdsOpeningMovieRoomHandoffTick = 0;
    gNdsOpeningMovieRoomHandoffScene = 0;
    gNdsOpeningPortraitsDispatchCount = 0;
    gNdsOpeningPortraitsStartResult = 0;
    gNdsOpeningPortraitsFuncStartResult = 0;
    gNdsOpeningPortraitsUpdateResult = 0;
    gNdsOpeningPortraitsTickCount = 0;
    gNdsOpeningPortraitsRelocResult = 0;
    gNdsOpeningPortraitsSpriteNormalizeCount = 0;
    gNdsOpeningPortraitsSpriteNormalizeFailCount = 0;
    gNdsOpeningPortraitsDrawResult = 0;
    gNdsOpeningPortraitsDrawBlocker = 0;
    gNdsOpeningPortraitsDrawCallbackCount = 0;
    gNdsOpeningPortraitsDrawVisibleSObjCount = 0;
    gNdsOpeningPortraitsDrawWidth = 0;
    gNdsOpeningPortraitsDrawHeight = 0;
    gNdsOpeningPortraitsDrawFormat = 0;
    gNdsOpeningPortraitsDrawSize = 0;
    gNdsOpeningPortraitsDrawBitmaps = 0;
    gNdsOpeningPortraitsDrawPixels = 0;
    gNdsOpeningPortraitsNextSceneResult = 0;
    gNdsOpeningPortraitsNextSceneKind = 0;
    gNdsOpeningMarioDispatchCount = 0;
    gNdsOpeningMarioStartResult = 0;
    gNdsOpeningMarioFuncStartResult = 0;
    gNdsOpeningMarioUpdateResult = 0;
    gNdsOpeningMarioTickCount = 0;
    gNdsOpeningMarioRelocResult = 0;
    gNdsOpeningMarioSpriteNormalizeCount = 0;
    gNdsOpeningMarioSpriteNormalizeFailCount = 0;
    gNdsOpeningMarioDrawResult = 0;
    gNdsOpeningMarioDrawBlocker = 0;
    gNdsOpeningMarioDrawCallbackCount = 0;
    gNdsOpeningMarioDrawVisibleSObjCount = 0;
    gNdsOpeningMarioDrawWidth = 0;
    gNdsOpeningMarioDrawHeight = 0;
    gNdsOpeningMarioDrawFormat = 0;
    gNdsOpeningMarioDrawSize = 0;
    gNdsOpeningMarioDrawBitmaps = 0;
    gNdsOpeningMarioDrawPixels = 0;
    gNdsOpeningMarioFighterDeferredResult = 0;
    gNdsOpeningMarioFighterDeferredTick = 0;
    gNdsOpeningMarioNextSceneResult = 0;
    gNdsOpeningMarioNextSceneKind = 0;
    gNdsOpeningNameSceneDispatchMask = 0;
    gNdsOpeningNameSceneFuncStartMask = 0;
    gNdsOpeningNameSceneUpdateMask = 0;
    gNdsOpeningNameSceneFighterDeferMask = 0;
    gNdsOpeningNameSceneNextMask = 0;
    gNdsOpeningNameSceneDrawMask = 0;
    gNdsOpeningNameSceneDispatchCount = 0;
    gNdsOpeningNameSceneLastKind = 0;
    gNdsOpeningNameSceneLastTick = 0;
    gNdsOpeningNameSceneLastNextKind = 0;
    gNdsOpeningNameSceneDrawResult = 0;
    gNdsOpeningNameSceneDrawBlocker = 0;
    gNdsOpeningNameSceneDrawCallbackCount = 0;
    gNdsOpeningNameSceneDrawVisibleSObjCount = 0;
    gNdsOpeningNameSceneDrawWidth = 0;
    gNdsOpeningNameSceneDrawHeight = 0;
    gNdsOpeningNameSceneDrawFormat = 0;
    gNdsOpeningNameSceneDrawSize = 0;
    gNdsOpeningNameSceneDrawBitmaps = 0;
    gNdsOpeningNameSceneDrawPixels = 0;
    gNdsOpeningMovieBridgeResult = 0;
    gNdsOpeningMovieBridgeMask = 0;
    gNdsOpeningMovieBridgeCount = 0;
    gNdsOpeningMovieBridgeLastKind = 0;
    gNdsOpeningMovieBridgeLastNextKind = 0;
    gNdsOpeningMovieActionPreviewResult = 0;
    gNdsOpeningMovieActionPreviewMask = 0;
    gNdsOpeningMovieActionPreviewCount = 0;
    gNdsOpeningMovieActionPreviewFrameCount = 0;
    gNdsOpeningMoviePresentFrameCount = 0;
    gNdsOpeningMovieActionPreviewPixels = 0;
    gNdsOpeningMovieActionPreviewSpriteNormalizeCount = 0;
    gNdsOpeningMovieActionPreviewSpriteNormalizeFailCount = 0;
    gNdsOpeningMovieActionPreviewLastKind = 0;
    gNdsOpeningMovieActionPreviewLastWidth = 0;
    gNdsOpeningMovieActionPreviewLastHeight = 0;
    gNdsOpeningMovieActionPreviewLastFormat = 0;
    gNdsOpeningMovieActionPreviewLastSize = 0;
    gNdsOpeningMovieTitleResult = 0;
    gNdsTitleRelocResult = 0;
    gNdsTitlePreviewResult = 0;
    gNdsTitleDrawResult = 0;
    gNdsTitleSpriteNormalizeCount = 0;
    gNdsTitleSpriteNormalizeFailCount = 0;
    gNdsTitleFireSpriteNormalizeCount = 0;
    gNdsTitleFireSpriteNormalizeFailCount = 0;
    gNdsTitleDrawVisibleSObjCount = 0;
    gNdsTitleDrawRenderableSObjCount = 0;
    gNdsTitleDrawSObjCount = 0;
    gNdsTitleDrawPixels = 0;
    gNdsTitleDrawLastWidth = 0;
    gNdsTitleDrawLastHeight = 0;
    gNdsTitleDrawLastFormat = 0;
    gNdsTitleDrawLastSize = 0;
    gNdsTitleOriginalStartResult = 0;
    gNdsTitleOriginalFuncStartResult = 0;
    gNdsTitleOriginalSetupMask = 0;
    gNdsTitleOriginalLoadedFileCount = 0;
    gNdsTitleOriginalGObjCount = 0;
    gNdsTitleOriginalCameraCount = 0;
    gNdsTitleOriginalMainGObjID = 0;
    gNdsTitleOriginalTransitionGObjID = 0;
    gNdsTitleOriginalDeferredMask = 0;
    gNdsTitleOriginalLogoFireResult = 0;
    gNdsTitleOriginalLogoFireMask = 0;
    gNdsTitleOriginalLogoFireGObjDelta = 0;
    gNdsTitleOriginalLogoFireLinkID = 0;
    gNdsTitleOriginalLogoFireDLLinkID = 0;
    gNdsTitleOriginalLogoFireCameraMaskLo = 0;
    gNdsTitleOriginalLogoFireParticleBank = 0;
    gNdsTitleOriginalFireResult = 0;
    gNdsTitleOriginalFireMask = 0;
    gNdsTitleOriginalFireGObjDelta = 0;
    gNdsTitleOriginalFireSObjDelta = 0;
    gNdsTitleOriginalFireGObjFlags = 0;
    gNdsTitleOriginalFireSObjCount = 0;
    gNdsTitleOriginalFireFrames = 0;
    gNdsTitleOriginalFireAlpha = 0;
    gNdsTitleOriginalUpdateResult = 0;
    gNdsTitleOriginalUpdateCount = 0;
    gNdsTitleOriginalLayout = 0;
    gNdsTitleOriginalTransitionTics = 0;
    gNdsTitleOriginalStartActorProcess = 0;
    gNdsTitleOriginalProceedScene = 0;
    gNdsTitleOriginalProceedWait = 0;
    gNdsVSModeOriginalStartResult = 0;
    gNdsVSModeOriginalFuncStartResult = 0;
    gNdsVSModeOriginalRelocResult = 0;
    gNdsVSModeOriginalSetupResult = 0;
    gNdsVSModeOriginalSetupMask = 0;
    gNdsVSModeOriginalLoadedFileCount = 0;
    gNdsVSModeOriginalGObjCount = 0;
    gNdsVSModeOriginalCameraCount = 0;
    gNdsVSModeOriginalSObjCount = 0;
    gNdsVSModeOriginalMainGObjID = 0;
    gNdsVSModeOriginalCursorIndex = 0;
    gNdsVSModeOriginalRule = 0;
    gNdsVSModeOriginalTime = 0;
    gNdsVSModeOriginalStock = 0;
    gNdsVSModeOriginalButtonMask = 0;
    gNdsVSModeOriginalDeferredMask = 0;
    gNdsVSModeStartTransitionResult = 0;
    gNdsVSModeStartTransitionMask = 0;
    gNdsVSModeStartTransitionUpdateCount = 0;
    gNdsVSModeStartTransitionInputMask = 0;
    gNdsVSModeStartTransitionScenePrevBefore = 0;
    gNdsVSModeStartTransitionSceneCurrBefore = 0;
    gNdsVSModeStartTransitionScenePrevAfterTap = 0;
    gNdsVSModeStartTransitionSceneCurrAfterTap = 0;
    gNdsVSModeStartTransitionScenePrevFinal = 0;
    gNdsVSModeStartTransitionSceneCurrFinal = 0;
    gNdsVSModeStartTransitionExitInterrupt = 0;
    gNdsVSModeStartTransitionTaskmanStatus = 0;
    gNdsVSModeStartTransitionSavedRule = 0;
    gNdsVSModeStartTransitionSavedTime = 0;
    gNdsVSModeStartTransitionSavedStock = 0;
    gNdsVSModeStartTransitionButtonMaskAfter = 0;
    gNdsVSModeStartTransitionCleanupCount = 0;
    gNdsPlayersVSOriginalStartResult = 0;
    gNdsPlayersVSOriginalFuncStartResult = 0;
    gNdsPlayersVSOriginalRelocResult = 0;
    gNdsPlayersVSOriginalSetupResult = 0;
    gNdsPlayersVSOriginalSetupMask = 0;
    gNdsPlayersVSOriginalLoadedFileCount = 0;
    gNdsPlayersVSOriginalGObjCount = 0;
    gNdsPlayersVSOriginalCameraCount = 0;
    gNdsPlayersVSOriginalSObjCount = 0;
    gNdsPlayersVSOriginalMainGObjID = 0;
    gNdsPlayersVSOriginalControllerOrderMask = 0;
    gNdsPlayersVSOriginalSlotKindMask = 0;
    gNdsPlayersVSOriginalSlotSelectedMask = 0;
    gNdsPlayersVSOriginalCursorCount = 0;
    gNdsPlayersVSOriginalPuckCount = 0;
    gNdsPlayersVSOriginalGateCount = 0;
    gNdsPlayersVSOriginalPortraitCount = 0;
    gNdsPlayersVSOriginalFigatreeHeapCount = 0;
    gNdsPlayersVSOriginalTime = 0;
    gNdsPlayersVSOriginalStock = 0;
    gNdsPlayersVSOriginalGameRule = 0;
    gNdsPlayersVSOriginalIsTeam = 0;
    gNdsPlayersVSOriginalIsStageSelect = 0;
    gNdsPlayersVSOriginalDeferredMask = 0;
    gNdsPlayersVSReadyTransitionResult = 0;
    gNdsPlayersVSReadyTransitionMask = 0;
    gNdsPlayersVSReadyTransitionUpdateCount = 0;
    gNdsPlayersVSReadyTransitionInputMask = 0;
    gNdsPlayersVSReadyTransitionScenePrevBefore = 0;
    gNdsPlayersVSReadyTransitionSceneCurrBefore = 0;
    gNdsPlayersVSReadyTransitionScenePrevFinal = 0;
    gNdsPlayersVSReadyTransitionSceneCurrFinal = 0;
    gNdsPlayersVSReadyTransitionPlayerCount = 0;
    gNdsPlayersVSReadyTransitionCpuCount = 0;
    gNdsPlayersVSReadyTransitionP0FKind = 0;
    gNdsPlayersVSReadyTransitionP1FKind = 0;
    gNdsPlayersVSReadyTransitionStageSelect = 0;
    gNdsPlayersVSReadyTransitionTaskmanStatus = 0;
    gNdsPlayersVSReadyTransitionCleanupCount = 0;
    gNdsMapsOriginalStartResult = 0;
    gNdsMapsOriginalFuncStartResult = 0;
    gNdsMapsOriginalRelocResult = 0;
    gNdsMapsOriginalSetupResult = 0;
    gNdsMapsOriginalSetupMask = 0;
    gNdsMapsOriginalLoadedFileCount = 0;
    gNdsMapsOriginalGObjCount = 0;
    gNdsMapsOriginalCameraCount = 0;
    gNdsMapsOriginalSObjCount = 0;
    gNdsMapsOriginalMainGObjID = 0;
    gNdsMapsOriginalCursorSlot = 0;
    gNdsMapsOriginalGroundKind = 0;
    gNdsMapsOriginalIsTrainingMode = 0;
    gNdsMapsOriginalPreviewDeferred = 0;
    gNdsMapsOriginalPreviewResult = 0;
    gNdsMapsOriginalPreviewMask = 0;
    gNdsMapsOriginalPreviewGObjCount = 0;
    gNdsMapsOriginalPreviewLayerGObjMask = 0;
    gNdsMapsOriginalPreviewWallpaperMade = 0;
    gNdsMapsOriginalPreviewModelMade = 0;
    gNdsMapsOriginalPreviewDObjCount = 0;
    gNdsMapsOriginalPreviewMObjCount = 0;
    gNdsMapsOriginalDeferredMask = 0;
    gNdsMapsSelectTransitionResult = 0;
    gNdsMapsSelectTransitionMask = 0;
    gNdsMapsSelectTransitionUpdateCount = 0;
    gNdsMapsSelectTransitionInputMask = 0;
    gNdsMapsSelectTransitionScenePrevBefore = 0;
    gNdsMapsSelectTransitionSceneCurrBefore = 0;
    gNdsMapsSelectTransitionScenePrevFinal = 0;
    gNdsMapsSelectTransitionSceneCurrFinal = 0;
    gNdsMapsSelectTransitionSelectedSlot = 0;
    gNdsMapsSelectTransitionSelectedGKind = 0;
    gNdsMapsSelectTransitionTaskmanStatus = 0;
    gNdsMapsSelectTransitionCleanupCount = 0;
    gNdsSCVSBattleOriginalStartResult = 0;
    gNdsSCVSBattleOriginalFuncStartResult = 0;
    gNdsSCVSBattleOriginalRelocResult = 0;
    gNdsSCVSBattleOriginalSetupResult = 0;
    gNdsSCVSBattleOriginalSetupMask = 0;
    gNdsSCVSBattleOriginalLoadedFileCount = 0;
    gNdsSCVSBattleOriginalGObjCount = 0;
    gNdsSCVSBattleOriginalCameraCount = 0;
    gNdsSCVSBattleOriginalMainGObjID = 0;
    gNdsSCVSBattleOriginalFighterGObjCount = 0;
    gNdsSCVSBattleOriginalActivePlayerMask = 0;
    gNdsSCVSBattleOriginalPlayerCount = 0;
    gNdsSCVSBattleOriginalActivePlayerCount = 0;
    gNdsSCVSBattleOriginalFighterCreateCount = 0;
    gNdsSCVSBattleOriginalP0FKind = 0xffffffffu;
    gNdsSCVSBattleOriginalP1FKind = 0xffffffffu;
    gNdsSCVSBattleOriginalP0LR = 0;
    gNdsSCVSBattleOriginalP1LR = 0;
    gNdsSCVSBattleOriginalCpuCount = 0;
    gNdsSCVSBattleOriginalGameRule = 0;
    gNdsSCVSBattleOriginalTime = 0;
    gNdsSCVSBattleOriginalStock = 0;
    gNdsSCVSBattleOriginalIsTeam = 0;
    gNdsSCVSBattleOriginalGKind = 0;
    gNdsSCVSBattleOriginalScenePrev = 0;
    gNdsSCVSBattleOriginalSceneCurr = 0;
    gNdsSCVSBattleOriginalUpdateResult = 0;
    gNdsSCVSBattleOriginalUpdateCount = 0;
    gNdsSCVSBattleLifecycleResult = 0;
    gNdsSCVSBattleLifecycleArenaAdapterCount = 0;
    gNdsSCVSBattleLifecycleTaskmanExitCount = 0;
    gNdsSCVSBattleLifecycleTaskmanStatus = 0;
    gNdsSCVSBattleLifecycleTimeLimit = 0;
    gNdsSCVSBattleLifecycleTimeRemain = 0;
    gNdsSCVSBattleLifecycleTimePassed = 0;
    gNdsSCVSBattleLifecycleGameStatus = 0;
    gNdsSCVSBattleLifecycleScenePrev = 0;
    gNdsSCVSBattleLifecycleSceneCurr = 0;
    gNdsSCVSBattleLifecycleIsSuddenDeath = 0;
    gNdsSCVSBattleCompatMask = 0;
    gNdsSCVSBattleCompatCameraMask = 0;
    gNdsSCVSBattleCompatInterfaceMask = 0;
    gNdsSCVSBattleCompatManagerMask = 0;
    gNdsSCVSBattleCompatAudioMask = 0;
    gNdsSCVSBattleCompatSpawnMask = 0;
    gNdsSCVSBattleLastAudioVolume = 0;
    gNdsSCVSBattleLastFGM = 0;
    gNdsStagePupupuRelocResult = 0;
    gNdsStagePupupuRelocAssetMask = 0;
    gNdsStagePupupuRelocDependencyMask = 0;
    gNdsStagePupupuExternalFixupCount = 0;
    gNdsStagePupupuExternalFixupFailCount = 0;
    gNdsStagePupupuInternalFixupCount = 0;
    gNdsStagePupupuMapHeaderOffset = 0;
    gNdsStagePupupuGroundDataPtrReady = 0;
    gNdsStagePupupuWallpaperPtrReady = 0;
    gNdsStagePupupuGeometryPtrReady = 0;
    gNdsStagePupupuMapNodesPtrReady = 0;
    gNdsStagePupupuLightAngleXBits = 0;
    gNdsStagePupupuLightAngleYBits = 0;
    gNdsStagePupupuBGM = 0;
    gNdsStagePupupuMapObjSourceCount = 0;
    gNdsStagePupupuMapObjDecodedMask = 0;
    gNdsStagePupupuMapObjDuplicateMask = 0;
    gNdsStagePupupuMapObjUnalignedReadCount = 0;
    gNdsStagePupupuMapObjSourceIndices[0] = 0xffffffffu;
    gNdsStagePupupuMapObjSourceIndices[1] = 0xffffffffu;
    gNdsStagePupupuMapObjSourceIndices[2] = 0xffffffffu;
    gNdsStagePupupuMapObjSourceIndices[3] = 0xffffffffu;
    gNdsStagePupupuMapObjXs[0] = 0;
    gNdsStagePupupuMapObjXs[1] = 0;
    gNdsStagePupupuMapObjXs[2] = 0;
    gNdsStagePupupuMapObjXs[3] = 0;
    gNdsStagePupupuMapObjYs[0] = 0;
    gNdsStagePupupuMapObjYs[1] = 0;
    gNdsStagePupupuMapObjYs[2] = 0;
    gNdsStagePupupuMapObjYs[3] = 0;
    gNdsFighterMarioFoxRelocResult = 0;
    gNdsFighterMarioFoxRelocAssetMask = 0;
    gNdsFighterMarioFoxRelocDependencyMask = 0;
    gNdsFighterMarioFoxLoadedFileCount = 0;
    gNdsFighterMarioFoxExternalFixupCount = 0;
    gNdsFighterMarioFoxExternalFixupFailCount = 0;
    gNdsFighterManagerResult = 0;
    gNdsFighterManagerMask = 0;
    gNdsFighterManagerExternMask = 0;
    gNdsFighterManagerStatusBufferMask = 0;
    gNdsFighterManagerFighterMask = 0;
    gNdsFighterManagerDataMask = 0;
    gNdsFighterManagerWaitMask = 0;
    gNdsFighterManagerEntryMask = 0;
    gNdsFighterManagerStatusBufferHitCount = 0;
    gNdsFighterManagerFighterCount = 0;
    gNdsFighterManagerFigatreeHeapSize = 0;
    gNdsFighterNaturalMotionResult = 0;
    gNdsFighterNaturalMotionSafeResult = 0;
    gNdsFighterNaturalMotionMask = 0;
    gNdsFighterNaturalMotionPrepared = 0;
    gNdsFighterNaturalMotionUpdateCount = 0;
    gNdsFighterNaturalMotionBaseVSBattleUpdateCount = 0;
    gNdsFighterNaturalMotionRunAllCount = 0;
    gNdsFighterNaturalMotionControllerReadCount = 0;
    gNdsFighterNaturalMotionManagerMask = 0;
    gNdsFighterNaturalMotionGObjCountBefore = 0;
    gNdsFighterNaturalMotionGObjCountAfter = 0;
    gNdsFighterNaturalMotionGObjDelta = 0;
    gNdsFighterNaturalMotionP0StatusStart = 0;
    gNdsFighterNaturalMotionP1StatusStart = 0;
    gNdsFighterNaturalMotionP0StatusFinal = 0;
    gNdsFighterNaturalMotionP1StatusFinal = 0;
    gNdsFighterNaturalMotionP0MotionFinal = 0;
    gNdsFighterNaturalMotionP1MotionFinal = 0;
    gNdsFighterNaturalMotionP0GAFinal = 0;
    gNdsFighterNaturalMotionP1GAFinal = 0;
    gNdsFighterNaturalMotionP0WaitFrameCount = 0;
    gNdsFighterNaturalMotionP1WaitFrameCount = 0;
    gNdsFighterNaturalMotionP0AnimAdvanceCount = 0;
    gNdsFighterNaturalMotionP1AnimAdvanceCount = 0;
    gNdsFighterNaturalMotionP0ValidJointCount = 0;
    gNdsFighterNaturalMotionP1ValidJointCount = 0;
    gNdsFighterNaturalMotionP0AnimStartBits = 0;
    gNdsFighterNaturalMotionP1AnimStartBits = 0;
    gNdsFighterNaturalMotionP0AnimFinalBits = 0;
    gNdsFighterNaturalMotionP1AnimFinalBits = 0;
    gNdsFighterNaturalMotionWalkInputFrame = 0;
    gNdsFighterNaturalMotionP0WalkFrameCount = 0;
    gNdsFighterNaturalMotionP1WalkFrameCount = 0;
    gNdsFighterNaturalMotionP0WalkStatus = 0;
    gNdsFighterNaturalMotionP1WalkStatus = 0;
    gNdsFighterNaturalMotionP0WalkMotion = 0;
    gNdsFighterNaturalMotionP1WalkMotion = 0;
    gNdsFighterNaturalMotionFigatreeAttachCount = 0;
    gNdsFighterNaturalMotionFigatreeNullCount = 0;
    gNdsFighterNaturalMotionFigatreeTableInvalidCount = 0;
    gNdsFighterNaturalMotionFigatreeAnimInvalidCount = 0;
    gNdsFighterNaturalMotionUnsafeCount = 0;
    gNdsFighterNaturalCombatPhase = 0;
    gNdsFighterNaturalCombatPhaseFrames = 0;
    gNdsFighterNaturalCombatStallCount = 0;
    gNdsFighterNaturalCombatApproachDXMilli = 0;
    gNdsFighterNaturalCombatAttackerSlot = 0;
    gNdsFighterNaturalCombatVictimSlot = 0;
    gNdsFighterNaturalCombatP0DashFrames = 0;
    gNdsFighterNaturalCombatP1DashFrames = 0;
    gNdsFighterNaturalCombatP0RunFrames = 0;
    gNdsFighterNaturalCombatP1RunFrames = 0;
    gNdsFighterNaturalCombatP0RunBrakeFrames = 0;
    gNdsFighterNaturalCombatP1RunBrakeFrames = 0;
    gNdsFighterNaturalCombatP0TurnFrames = 0;
    gNdsFighterNaturalCombatP1TurnFrames = 0;
    gNdsFighterNaturalCombatP0HitlagFrames = 0;
    gNdsFighterNaturalCombatP1HitlagFrames = 0;
    gNdsFighterNaturalCombatAttackStatusFrames = 0;
    gNdsFighterNaturalCombatAttackMotionFinal = 0;
    gNdsFighterNaturalCombatHitboxActiveFrames = 0;
    gNdsFighterNaturalCombatAttackRetryCount = 0;
    gNdsFighterNaturalCombatVictimDamageStatus = 0;
    gNdsFighterNaturalCombatVictimDamageFrames = 0;
    gNdsFighterNaturalCombatVictimStartPercent = 0;
    gNdsFighterNaturalCombatVictimFinalPercent = 0;
    gNdsFighterNaturalCombatVictimKnockbackMilli = 0;
    gNdsFighterNaturalCombatVictimRecoverWaitFrames = 0;
    gNdsFighterNaturalCombatGuardOnFrames = 0;
    gNdsFighterNaturalCombatGuardFrames = 0;
    gNdsFighterNaturalCombatGuardOffFrames = 0;
    gNdsFighterProjectileProofResult = 0;
    gNdsFighterProjectileProofMask = 0;
    gNdsFighterProjectileProofActorSlot = 0;
    gNdsFighterProjectileProofActorKind = 0;
    gNdsFighterProjectileProofBPressFrames = 0;
    gNdsFighterProjectileProofSpecialStatusFrames = 0;
    gNdsFighterProjectileProofSpecialMotion = 0;
    gNdsFighterProjectileProofAccessoryFrames = 0;
    gNdsFighterProjectileProofFlag0Frames = 0;
    gNdsFighterProjectileProofSpawnCallCount = 0;
    gNdsFighterProjectileProofSpawnSuccessCount = 0;
    gNdsFighterProjectileProofUpdateDestroyCount = 0;
    gNdsFighterProjectileProofMapDestroyCount = 0;
    gNdsFighterProjectileProofHitDestroyCount = 0;
    gNdsFighterProjectileProofWeaponFrames = 0;
    gNdsFighterProjectileProofWeaponCountMax = 0;
    gNdsFighterProjectileProofKindMask = 0;
    gNdsFighterProjectileProofAttackStateMask = 0;
    gNdsFighterProjectileProofDamageMax = 0;
    gNdsFighterProjectileProofLifetimeMax = 0;
    gNdsFighterProjectileProofMapMask = 0;
    ndsCollisionRuntimeDiagnosticsReset();
    gNdsFighterReflectorProofResult = 0;
    gNdsFighterReflectorProofMask = 0;
    gNdsFighterReflectorProofFoxSlot = 0;
    gNdsFighterReflectorProofProjectileSlot = 0;
    gNdsFighterReflectorProofDownBPressFrames = 0;
    gNdsFighterReflectorProofStartFrames = 0;
    gNdsFighterReflectorProofLoopFrames = 0;
    gNdsFighterReflectorProofHitFrames = 0;
    gNdsFighterReflectorProofIsReflectFrames = 0;
    gNdsFighterReflectorProofReflectLRBeforeHit = 0;
    gNdsFighterReflectorProofReflectLRClearFrames = 0;
    gNdsFighterReflectorProofHitSetCallCount = 0;
    gNdsFighterReflectorProofFireballProcCount = 0;
    gNdsFighterReflectorProofFireballVelXBefore = 0;
    gNdsFighterReflectorProofFireballVelXAfter = 0;
    gNdsFighterReflectorProofFireballOwnerKind = 0;
    gNdsFighterReflectorProofFireballCanReflect = 0;
    gNdsFighterReflectorProofFireballCanAbsorb = 0;
    gNdsFighterReflectorProofFireballCanShield = 0;
    gNdsFighterReflectorProofFireballAttackCount = 0;
    gNdsFighterReflectorProofFireballDamage = 0;
    gNdsFighterReflectorProofFireballSizeMilli = 0;
    gNdsFighterReflectorProofFireballDXMilli = 0;
    gNdsFighterReflectorProofFireballDYMilli = 0;
    gNdsFighterReflectorProofSpecialSizeMilli = 0;
    gNdsFighterReflectorProofSpecialResist = 0;
    gNdsFighterSpecialsProofMask = 0;
    gNdsFighterSpecialsProofPhase = 0;
    gNdsFighterSpecialsProofPhaseFrames = 0;
    gNdsFighterSpecialsMarioSlot = 0;
    gNdsFighterSpecialsFoxSlot = 0;
    gNdsFighterSpecialsMarioHiPressFrames = 0;
    gNdsFighterSpecialsMarioHiFrames = 0;
    gNdsFighterSpecialsMarioAirHiFrames = 0;
    gNdsFighterSpecialsMarioFallSpecialFrames = 0;
    gNdsFighterSpecialsMarioLandingFallSpecialFrames = 0;
    gNdsFighterSpecialsMarioHiWaitFrames = 0;
    gNdsFighterSpecialsMarioHiRootYMilli = 0;
    gNdsFighterSpecialsMarioLwPressFrames = 0;
    gNdsFighterSpecialsMarioLwFrames = 0;
    gNdsFighterSpecialsMarioAirLwFrames = 0;
    gNdsFighterSpecialsMarioLwDustEffectCount = 0;
    gNdsFighterSpecialsMarioLwWaitFrames = 0;
    gNdsFighterSpecialsFoxHiPressFrames = 0;
    gNdsFighterSpecialsFoxHiStartFrames = 0;
    gNdsFighterSpecialsFoxHiHoldFrames = 0;
    gNdsFighterSpecialsFoxHiTravelFrames = 0;
    gNdsFighterSpecialsFoxHiEndFrames = 0;
    gNdsFighterSpecialsFoxHiBoundFrames = 0;
    gNdsFighterSpecialsFoxHiWaitFrames = 0;
    gNdsFighterSpecialsFoxHiRootYMilli = 0;
    gNdsFighterNaturalMovesetMask = 0;
    gNdsFighterNaturalMovesetPhase = 0;
    gNdsFighterNaturalMovesetPhaseFrames = 0;
    gNdsFighterNaturalMovesetTiltS3Frames = 0;
    gNdsFighterNaturalMovesetTiltHi3Frames = 0;
    gNdsFighterNaturalMovesetTiltLw3Frames = 0;
    gNdsFighterNaturalMovesetTiltHitboxFrames = 0;
    gNdsFighterNaturalMovesetSmashFrames = 0;
    gNdsFighterNaturalMovesetSmashHitboxFrames = 0;
    gNdsFighterNaturalMovesetAerialFrames = 0;
    gNdsFighterNaturalMovesetAerialHitboxFrames = 0;
    gNdsFighterNaturalMovesetLandingFrames = 0;
    gNdsFighterNaturalMovesetCatchFrames = 0;
    gNdsFighterNaturalMovesetCatchWaitFrames = 0;
    gNdsFighterNaturalMovesetThrowFrames = 0;
    gNdsFighterNaturalMovesetThrownFrames = 0;
    gNdsFighterNaturalMovesetThrowRecoverFrames = 0;
    gNdsFighterNaturalMovesetThrowDamageBefore = 0;
    gNdsFighterNaturalMovesetThrowDamageAfter = 0;
    gNdsFighterNaturalMovesetAttackerStatus = 0;
    gNdsFighterNaturalMovesetAttackerMotion = 0;
    gNdsFighterNaturalMovesetAttackerGA = 0;
    gNdsFighterNaturalMovesetAttackerRootYMilli = 0;
    gNdsFighterNaturalMovesetVictimStatus = 0;
    gNdsFighterNaturalMovesetVictimMotion = 0;
    gNdsFighterNaturalMovesetVictimGA = 0;
    gNdsFighterNaturalMovesetVictimRootYMilli = 0;
    gNdsFighterBattlePlayableResult = 0;
    gNdsFighterBattlePlayableMask = 0;
    gNdsFighterBattlePlayableVictimSlot = 0;
    gNdsFighterBattlePlayableVictimStockStart = 0;
    gNdsFighterBattlePlayableVictimStockFinal = 0;
    gNdsFighterBattlePlayableBattleStockStart = 0;
    gNdsFighterBattlePlayableBattleStockFinal = 0;
    gNdsFighterBattlePlayableFallsStart = 0;
    gNdsFighterBattlePlayableFallsFinal = 0;
    gNdsFighterBattlePlayableDeadFrames = 0;
    gNdsFighterBattlePlayableRebirthDownFrames = 0;
    gNdsFighterBattlePlayableRebirthStandFrames = 0;
    gNdsFighterBattlePlayableRebirthWaitFrames = 0;
    gNdsFighterBattlePlayableFallAfterRebirthFrames = 0;
    gNdsFighterBattlePlayableWaitAfterRebirthFrames = 0;
    gNdsFighterBattlePlayableFinalStatus = 0;
    gNdsFighterBattlePlayableFinalGA = 0;
    gNdsFighterBattlePlayableFinalFloor = 0;
    gNdsFighterBattlePlayableFinalIsRebirth = 0;
    gNdsFighterBattlePlayableFinalIsGhost = 0;
    gNdsFighterBattlePlayableFinalCameraMode = 0;
    gNdsFighterBattlePlayableKOStickFrames = 0;
    gNdsFighterBattlePlayableMapCallCount = 0;
    gNdsFighterBattlePlayableMapHitCount = 0;
    gNdsFighterBattlePlayableMapFloorHitCount = 0;
    gNdsFighterBattlePlayableMapCliffHitCount = 0;
    gNdsFighterBattlePlayableMapCeilHitCount = 0;
    gNdsFighterBattlePlayableMapLastMaskStat = 0;
    gNdsFighterBattlePlayableMapLastMaskCurr = 0;
    gNdsFighterBattlePlayableFinalXMilli = 0;
    gNdsFighterBattlePlayableFinalYMilli = 0;
    gNdsFighterBattlePlayableFinalVelXMilli = 0;
    gNdsFighterBattlePlayableFinalVelYMilli = 0;
    gNdsFighterBattlePlayableFinalFloorDistMilli = 0;
    gNdsFTComputerSetupCount = 0;
    gNdsFTComputerDamageDetectCount = 0;
    gNdsFTComputerProcessCount = 0;
    gNdsFTComputerTargetFrames = 0;
    gNdsFTComputerObjectiveMask = 0;
    gNdsFTComputerBehaviorMask = 0;
    gNdsFTComputerInputChangeCount = 0;
    gNdsFTComputerStickFrames = 0;
    gNdsFTComputerButtonAFrames = 0;
    gNdsFTComputerButtonBFrames = 0;
    gNdsFTComputerButtonZFrames = 0;
    gNdsFTComputerAttackFrames = 0;
    gNdsFTComputerHitboxFrames = 0;
    gNdsFTComputerGuardFrames = 0;
    gNdsFTComputerRecoveryFrames = 0;
    gNdsFTComputerStatusChangeCount = 0;
    gNdsFTComputerFinalStatus = 0;
    gNdsFTComputerFinalGA = 0;
    gNdsFTComputerFinalInputKind = 0;
    gNdsFTComputerMarioDamageMax = 0;
    gNdsFTComputerFloorLineCount = 0;
    gNdsFTComputerStartXMilli = 0;
    gNdsFTComputerMinXMilli = 0;
    gNdsFTComputerMaxXMilli = 0;
    gNdsFTComputerFinalXMilli = 0;
    gNdsBattlePlayablePacingResult = 0;
    gNdsBattlePlayablePacingMode = 0;
    gNdsBattlePlayablePacingLogicFrames = 0;
    gNdsBattlePlayablePacingPresentedFrames = 0;
    gNdsBattlePlayablePacingDrawCalls = 0;
    gNdsBattlePlayablePacingTimerTicks = 0;
    gNdsBattlePlayablePacingPresentFpsX10 = 0;
    gNdsBattlePlayablePacingLogicFpsX10 = 0;
    gNdsBattlePlayablePacingVBlankStart = 0;
    gNdsBattlePlayablePacingVBlanks = 0;
    gNdsBattlePlayablePacingRestartRequested = 0;
    gNdsBattlePlayablePacingPresentIntervalMin = 0;
    gNdsBattlePlayablePacingPresentIntervalMax = 0;
    gNdsBattlePlayablePacingCadenceViolationCount = 0;
    gNdsBattlePlayablePacingPhasePresentCount[0] = 0;
    gNdsBattlePlayablePacingPhasePresentCount[1] = 0;
    gNdsBattlePlayablePacingPhasePresentCount[2] = 0;
    gNdsBattlePlayablePacingPhasePresentCount[3] = 0;
    gNdsBattlePlayablePacingPhasePresentCount[4] = 0;
    gNdsBattlePlayablePacingPhaseSlipCount[0] = 0;
    gNdsBattlePlayablePacingPhaseSlipCount[1] = 0;
    gNdsBattlePlayablePacingPhaseSlipCount[2] = 0;
    gNdsBattlePlayablePacingPhaseSlipCount[3] = 0;
    gNdsBattlePlayablePacingPhaseSlipCount[4] = 0;
#if (NDS_HARNESS_FAST_LOGIC == 0) && \
    (NDS_RENDERER_HW_TRIANGLES != 0) && \
    (NDS_DEV_LIVE_INPUT_PREVIEW != 0)
    gNdsBuildModeCanonicalWord = NDS_BUILD_MODE_CANO_WORD;
    gNdsBuildModeShippedWord = NDS_BUILD_MODE_SHIP_WORD;
#else
    gNdsBuildModeCanonicalWord = 0;
    gNdsBuildModeShippedWord = 0;
#endif
#if NDS_HARNESS_FAST_LOGIC != 0
    gNdsBuildModeFastWord = NDS_BUILD_MODE_FAST_WORD;
#else
    gNdsBuildModeFastWord = 0;
#endif
    gNdsRendererProfileFrameCount = 0;
    gNdsRendererProfileUpdateTicks = 0;
    gNdsRendererProfilePresentTicks = 0;
    gNdsRendererProfileDrawTicks = 0;
    gNdsRendererProfileHudTicks = 0;
    gNdsRendererProfileStageAdapterTicks = 0;
    gNdsRendererProfileMaterialTicks = 0;
    gNdsRendererProfileMatrixTicks = 0;
    gNdsRendererProfileDLTicks = 0;
    gNdsRendererProfileTextureTicks = 0;
    gNdsRendererProfileTextureConvertTicks = 0;
    gNdsRendererProfileTextureUploadTicks = 0;
    gNdsRendererProfileTextureUploads = 0;
    gNdsRendererProfileTextureUploadBytes = 0;
    gNdsRendererProfileTextureCi4DirectPixels = 0;
    gNdsRendererProfileTextureBinds = 0;
    gNdsRendererProfileTextureSourceTexels = 0;
    gNdsRendererProfileTextureGreenTexels = 0;
    gNdsRendererProfileTextureNonWhiteTexels = 0;
    gNdsRendererProfileTexturedVertexCount = 0;
    gNdsRendererProfileTextureSampleCount = 0;
    gNdsRendererProfileTextureSampleGreenCount = 0;
    gNdsRendererProfileTextureSampleNonWhiteCount = 0;
    gNdsRendererProfileTextureCacheAliasAvoidCount = 0;
    gNdsRendererProfileTextureLookupCallCount = 0;
    gNdsRendererProfileTextureLookupProbeCount = 0;
    gNdsRendererProfileTextureLookupActiveHitCount = 0;
    gNdsRendererProfileTextureLookupTableHitCount = 0;
    gNdsRendererProfileTextureLookupMissCount = 0;
    gNdsRendererProfileTexel1CompositeCount = 0;
    gNdsRendererProfileTexel1LoadMatchCount = 0;
    gNdsRendererProfileTexel1RejectCount = 0;
    gNdsRendererProfileTexel1RejectReasonMask = 0;
    gNdsRendererProfileTexel1LastFraction = 0;
    gNdsRendererProfileTexel1LastImage0 = 0;
    gNdsRendererProfileTexel1LastImage1 = 0;
    gNdsRendererProfileTexel1LastTileState = 0;
    gNdsRendererProfileTexel1LastPrimaryState = 0;
    gNdsRendererProfileTexel1FractionRefreshCount = 0;
    gNdsRendererProfileTextureCacheEvictCount = 0;
    gNdsRendererProfileTextureCoordMinS = 32767;
    gNdsRendererProfileTextureCoordMaxS = -32768;
    gNdsRendererProfileTextureCoordMinT = 32767;
    gNdsRendererProfileTextureCoordMaxT = -32768;
    gNdsRendererProfileTextureConvertFormatMask = 0;
    gNdsRendererProfileTextureBindFormatMask = 0;
    gNdsRendererProfileTexturePaletteFormatMask = 0;
    gNdsRendererProfileTextureRejectFormatMask = 0;
    gNdsRendererProfileTextureRejectReasonMask = 0;
    gNdsRendererProfileTextureLaneLayoutMask = 0;
    gNdsRendererProfileTextureLaneByteAccessCount = 0;
    gNdsRendererProfileTextureLaneHalfwordAccessCount = 0;
    gNdsRendererProfileTextureLaneByteFormatMask = 0;
    gNdsRendererProfileTextureLaneHalfwordFormatMask = 0;
    gNdsRendererProfileTextureLaneByteMap = 0;
    gNdsRendererProfileTextureLaneHalfwordMap = 0;
    gNdsRendererProfileUseTextureRejectNoStatsCount = 0;
    gNdsRendererProfileUseTextureRejectStateOffCount = 0;
    gNdsRendererProfileUseTextureRejectNoCombineCount = 0;
    gNdsRendererProfileUseTextureRejectPrimitiveDecalCount = 0;
    gNdsRendererProfileUseTextureRejectNoTexel0Count = 0;
    gNdsRendererProfileUseTextureImplicitOnCount = 0;
    gNdsRendererProfileUseTextureRejectFirstReason = 0;
    gNdsRendererProfileUseTextureRejectFirstFlags = 0;
    gNdsRendererProfileUseTextureRejectFirstW0 = 0;
    gNdsRendererProfileUseTextureRejectFirstW1 = 0;
    gNdsRendererProfileUseTextureRejectFirstGeometry = 0;
    gNdsRendererProfileCombineModeCount = 0;
    gNdsRendererProfileCombineModeDistinctCount = 0;
    gNdsRendererProfileCombineMode0W0 = 0;
    gNdsRendererProfileCombineMode0W1 = 0;
    gNdsRendererProfileCombineMode1W0 = 0;
    gNdsRendererProfileCombineMode1W1 = 0;
    gNdsRendererProfileCombineMode2W0 = 0;
    gNdsRendererProfileCombineMode2W1 = 0;
    gNdsRendererProfileCombineMode3W0 = 0;
    gNdsRendererProfileCombineMode3W1 = 0;
    gNdsRendererProfileLitShadeCombineCount = 0;
    gNdsRendererProfileMaterialCombineCount = 0;
    gNdsRendererProfileProjectedSubmitFallbackCount = 0;
    gNdsRendererProfileNearPlaneTriangleRejectCount = 0;
    gNdsRendererProfileLightColorCommands = 0;
    gNdsRendererProfileLightDirectionCommands = 0;
    gNdsRendererProfileLightFallbackCount = 0;
    gNdsRendererProfileHardwareVertices = 0;
    gNdsRendererProfileHardwareTriangles = 0;
    gNdsRendererProfileHardwareBatchBeginCount = 0;
    gNdsRendererProfileHardwareBatchReuseCount = 0;
    gNdsRendererProfileHardwareBatchEndCount = 0;
    gNdsRendererProfileTexturePrepareCount = 0;
    gNdsRendererProfileTexturePrepareReuseCount = 0;
    gNdsRendererProfileImmutableListCount = 0;
    gNdsRendererProfileTrustedCommandCount = 0;
    gNdsRendererProfileValidatedCommandCount = 0;
    gNdsRendererProfileTriangleRunReuseCount = 0;
    gNdsRendererProfileTriangleSubmitTicks = 0;
    gNdsRendererProfileVertexSubmitTicks = 0;
    gNdsRendererProfileCi4LutBuildCount = 0;
    gNdsRendererProfileCi4LutReuseCount = 0;
    gNdsRendererProfileCi4IndexCacheBuildCount = 0;
    gNdsRendererProfileCi4IndexCacheReuseCount = 0;
    gNdsRendererProfileCi4RepresentativePixelCount = 0;
    gNdsRendererProfileCi4ReusePixelCount = 0;
    gNdsRendererProfileHardwareOverLimit = 0;
    gNdsRendererProfileOracleSamples = 0;
    gNdsRendererProfileOracleMismatches = 0;
    gNdsRendererProfileOracleMaxDelta = 0;
    gNdsRendererProfileMatrixLoadCount = 0;
    gNdsRendererProfileCameraMatrixCacheHitCount = 0;
    gNdsRendererProfileCameraMatrixCacheMissCount = 0;
    gNdsRendererProfileCameraMatrixCacheOverflowCount = 0;
    gNdsRendererProfileDObjWorldCacheHitCount = 0;
    gNdsRendererProfileDObjWorldCacheMissCount = 0;
    gNdsRendererProfileDObjWorldCacheOverflowCount = 0;
    gNdsRendererProfileStageWorldPersistentHitCount = 0;
    gNdsRendererProfileStageWorldPersistentMissCount = 0;
    gNdsRendererProfileStageWorldPersistentRejectCount = 0;
    gNdsRendererProfileStageWorldPersistentOverflowCount = 0;
    gNdsRendererProfileStageWorldPersistentOracleSampleCount = 0;
    gNdsRendererProfileStageWorldPersistentOracleMismatchCount = 0;
    gNdsRendererProfileAffineMatrixSamples = 0;
    gNdsRendererProfileAffineMatrixMismatches = 0;
    gNdsRendererProfileAffineMatrixMaxDelta = 0;
    gNdsRendererProfileRawCurrentCandidateCount = 0;
    gNdsRendererProfileRawCurrentRangeRejectCount = 0;
    gNdsRendererProfileRawCrossMatrixCount = 0;
    gNdsRendererProfileSubmitRawCurrentCount = 0;
    gNdsRendererProfileSubmitRawSnapshotCount = 0;
    gNdsRendererProfileSubmitProjectedCrossCount = 0;
    gNdsRendererProfileSubmitProjectedNoZCount = 0;
    gNdsRendererProfileSubmitProjectedDecalCount = 0;
    gNdsRendererProfileSubmitProjectedPrimDepthCount = 0;
    gNdsRendererProfileSubmitProjectedRangeOrMatrixCount = 0;
    gNdsRendererProfileSubmitRejectCount = 0;
    gNdsRendererProfileHardwareDivideSummary = 0;
    gNdsRendererProfileSourceVertexLoadCount = 0;
    gNdsRendererProfileCPUTransformCount = 0;
    gNdsRendererProfileTransformCacheHitCount = 0;
    gNdsRendererProfileMatrixSnapshotCreateCount = 0;
    gNdsRendererProfileMatrixSnapshotReuseCount = 0;
    gNdsRendererProfileMatrixSnapshotOverflowCount = 0;
    gNdsRendererProfileMatrixPosTestSamples = 0;
    gNdsRendererProfileMatrixPosTestMismatches = 0;
    gNdsRendererProfileMatrixPosTestMaxError = 0;
    gNdsRendererProfileMatrixPosTestWSignMismatches = 0;
    gNdsRendererProfileMatrixPosTestClipMismatches = 0;
    gNdsRendererProfileMatrixPosTestMatrixWordSamples = 0;
    gNdsRendererProfileMatrixPosTestDropped = 0;
    gNdsRendererProfileMatrixScaleWorld = 0;
    gNdsRendererProfileProjectionM00 = 0;
    gNdsRendererProfileProjectionM11 = 0;
    gNdsRendererProfileProjectionM22 = 0;
    gNdsRendererProfileProjectionM32 = 0;
    gNdsRendererProfileModelviewM00 = 0;
    gNdsRendererProfileModelviewM11 = 0;
    gNdsRendererProfileModelviewM22 = 0;
    gNdsRendererProfileModelviewM30 = 0;
    gNdsRendererProfileModelviewM31 = 0;
    gNdsRendererProfileModelviewM32 = 0;
    gNdsRendererProfileRawVertexMinX = 0;
    gNdsRendererProfileRawVertexMaxX = 0;
    gNdsRendererProfileRawVertexMinY = 0;
    gNdsRendererProfileRawVertexMaxY = 0;
    gNdsRendererProfileRawVertexMinZ = 0;
    gNdsRendererProfileRawVertexMaxZ = 0;
    gNdsRendererProfileHWVertexMinX = 0;
    gNdsRendererProfileHWVertexMaxX = 0;
    gNdsRendererProfileHWVertexMinY = 0;
    gNdsRendererProfileHWVertexMaxY = 0;
    gNdsRendererProfileHWVertexMinZ = 0;
    gNdsRendererProfileHWVertexMaxZ = 0;
    gNdsRendererProfileHWVertexSaturateCount = 0;
    gNdsRendererDepthStageSamples = 0;
    gNdsRendererDepthStageMin = 0;
    gNdsRendererDepthStageMax = 0;
    gNdsRendererDepthStageWMin = 0;
    gNdsRendererDepthStageWMax = 0;
    gNdsRendererDepthFighterP0Samples = 0;
    gNdsRendererDepthFighterP0Min = 0;
    gNdsRendererDepthFighterP0Max = 0;
    gNdsRendererDepthFighterP0WMin = 0;
    gNdsRendererDepthFighterP0WMax = 0;
    gNdsRendererDepthFighterP1Samples = 0;
    gNdsRendererDepthFighterP1Min = 0;
    gNdsRendererDepthFighterP1Max = 0;
    gNdsRendererDepthFighterP1WMin = 0;
    gNdsRendererDepthFighterP1WMax = 0;
    gNdsIFCommonHUDRecordCount = 0;
    gNdsIFCommonHUDObjectMask = 0;
    gNdsIFCommonHUDP0DamageCurrent = 0;
    gNdsIFCommonHUDP1DamageCurrent = 0;
    gNdsIFCommonHUDP0DamageMax = 0;
    gNdsIFCommonHUDP1DamageMax = 0;
    gNdsIFCommonHUDP0DigitCount = 0;
    gNdsIFCommonHUDP1DigitCount = 0;
    gNdsIFCommonHUDP0Digits = 0;
    gNdsIFCommonHUDP1Digits = 0;
    gNdsIFCommonHUDP0StockCurrent = 0;
    gNdsIFCommonHUDP1StockCurrent = 0;
    gNdsIFCommonHUDP0StockMin = 0;
    gNdsIFCommonHUDP1StockMin = 0;
    gNdsIFCommonHUDP0StockMax = 0;
    gNdsIFCommonHUDP1StockMax = 0;
    gNdsIFCommonHUDActivePlayerMask = 0;
    gNdsIFCommonHUDShowDamageMask = 0;
    gNdsIFCommonHUDSingleStockMask = 0;
    gNdsIFCommonHUDCPUPlayerMask = 0;
    gNdsIFCommonHUDP0FighterKind = 0;
    gNdsIFCommonHUDP1FighterKind = 0;
    gNdsIFCommonHUDP0Level = 0;
    gNdsIFCommonHUDP1Level = 0;
    gNdsIFCommonHUDP0LowerStock = 0;
    gNdsIFCommonHUDP1LowerStock = 0;
    gNdsIFCommonHUDTimeRemain = 0;
    gNdsIFCommonHUDTimerLimit = 0;
    gNdsIFCommonHUDTimerStarted = 0;
    gNdsIFCommonHUDGameStatus = 0;
    gNdsIFCommonHUDLowerRouteMask = 0;
    gNdsIFCommonHUDLowerRouteCount = 0;
    gNdsIFCommonHUDLowerTimerRouteCount = 0;
    gNdsIFCommonHUDLowerStockRouteCount = 0;
    gNdsIFCommonHUDTopGenericPassCount = 0;
    gNdsFighterMarioFoxModelResult = 0;
    gNdsFighterMarioFoxGObjResult = 0;
    gNdsFighterMarioFoxSetupMask = 0;
    gNdsFighterMarioMainMotionPtrReady = 0;
    gNdsFighterMarioMainPtrReady = 0;
    gNdsFighterMarioModelPtrReady = 0;
    gNdsFighterMarioAttrPtrReady = 0;
    gNdsFighterMarioCommonPartsReady = 0;
    gNdsFighterFoxMainMotionPtrReady = 0;
    gNdsFighterFoxMainPtrReady = 0;
    gNdsFighterFoxModelPtrReady = 0;
    gNdsFighterFoxAttrPtrReady = 0;
    gNdsFighterFoxCommonPartsReady = 0;
    gNdsFighterModelRealGObjCount = 0;
    gNdsFighterModelStubGObjCount = 0;
    gNdsFighterModelProcessDeferredCount = 0;
    gNdsFighterModelP0FKind = 0xffffffffu;
    gNdsFighterModelP0GObjID = 0;
    gNdsFighterModelP0TopDObjReady = 0;
    gNdsFighterModelP0ModelDObjCount = 0;
    gNdsFighterModelP0MObjCount = 0;
    gNdsFighterModelP0AObjCount = 0;
    gNdsFighterModelP0DisplayAttached = 0;
    gNdsFighterModelP1FKind = 0xffffffffu;
    gNdsFighterModelP1GObjID = 0;
    gNdsFighterModelP1TopDObjReady = 0;
    gNdsFighterModelP1ModelDObjCount = 0;
    gNdsFighterModelP1MObjCount = 0;
    gNdsFighterModelP1AObjCount = 0;
    gNdsFighterModelP1DisplayAttached = 0;
    gNdsFighterMarioFoxStructResult = 0;
    gNdsFighterMarioFoxJointResult = 0;
    gNdsFighterMarioFoxStateResult = 0;
    gNdsFighterMarioFoxStructMask = 0;
    gNdsFighterMarioFoxStructPoolUsedMask = 0;
    gNdsFighterMarioFoxStructCount = 0;
    gNdsFighterStructP0PtrReady = 0;
    gNdsFighterStructP1PtrReady = 0;
    gNdsFighterStructP0UserDataPtrReady = 0;
    gNdsFighterStructP1UserDataPtrReady = 0;
    gNdsFighterStructP0FtGetStructReady = 0;
    gNdsFighterStructP1FtGetStructReady = 0;
    gNdsFighterStructP0FKind = 0xffffffffu;
    gNdsFighterStructP1FKind = 0xffffffffu;
    gNdsFighterStructP0PKind = 0xffffffffu;
    gNdsFighterStructP1PKind = 0xffffffffu;
    gNdsFighterStructP0Player = 0xffffffffu;
    gNdsFighterStructP1Player = 0xffffffffu;
    gNdsFighterStructP0LR = 0;
    gNdsFighterStructP1LR = 0;
    gNdsFighterStructP0Stock = 0;
    gNdsFighterStructP1Stock = 0;
    gNdsFighterStructP0Detail = 0;
    gNdsFighterStructP1Detail = 0;
    gNdsFighterStructP0Costume = 0;
    gNdsFighterStructP1Costume = 0;
    gNdsFighterStructP0Shade = 0;
    gNdsFighterStructP1Shade = 0;
    gNdsFighterStructP0AttrReady = 0;
    gNdsFighterStructP1AttrReady = 0;
    gNdsFighterStructP0FigatreeHeapReady = 0;
    gNdsFighterStructP1FigatreeHeapReady = 0;
    gNdsFighterStructP0ControllerReady = 0;
    gNdsFighterStructP1ControllerReady = 0;
    gNdsFighterStructP0InputMaskA = 0;
    gNdsFighterStructP1InputMaskA = 0;
    gNdsFighterStructP0InputMaskB = 0;
    gNdsFighterStructP1InputMaskB = 0;
    gNdsFighterStructP0InputMaskZ = 0;
    gNdsFighterStructP1InputMaskZ = 0;
    gNdsFighterStructP0InputMaskL = 0;
    gNdsFighterStructP1InputMaskL = 0;
    gNdsFighterStructP0TopJointReady = 0;
    gNdsFighterStructP1TopJointReady = 0;
    gNdsFighterStructP0JointCount = 0;
    gNdsFighterStructP1JointCount = 0;
    gNdsFighterStructP0CommonJointCount = 0;
    gNdsFighterStructP1CommonJointCount = 0;
    gNdsFighterStructP0CollTranslateReady = 0;
    gNdsFighterStructP1CollTranslateReady = 0;
    gNdsFighterStructP0CollLRReady = 0;
    gNdsFighterStructP1CollLRReady = 0;
    gNdsFighterStructP0StatusID = 0;
    gNdsFighterStructP1StatusID = 0;
    gNdsFighterStructP0StatusTotalTics = 0;
    gNdsFighterStructP1StatusTotalTics = 0;
    gNdsFighterStructProcessAttachCount = 0;
    gNdsFighterStructStatusSetCount = 0;
    gNdsFighterStructDisplayProbeCount = 0;
    gNdsFighterMarioFoxInitResult = 0;
    gNdsFighterMarioFoxCollResult = 0;
    gNdsFighterMarioFoxDeferResult = 0;
    gNdsFighterMarioFoxInitMask = 0;
    gNdsFighterMarioFoxInitDeferredMask = 0;
    gNdsFighterMarioFoxInitCount = 0;
    gNdsFighterInitP0FKind = 0xffffffffu;
    gNdsFighterInitP1FKind = 0xffffffffu;
    gNdsFighterInitP0PercentDamage = 0;
    gNdsFighterInitP1PercentDamage = 0;
    gNdsFighterInitP0ShieldHealth = 0;
    gNdsFighterInitP1ShieldHealth = 0;
    gNdsFighterInitP0GA = 0xffffffffu;
    gNdsFighterInitP1GA = 0xffffffffu;
    gNdsFighterInitP0JumpsUsed = 0xffffffffu;
    gNdsFighterInitP1JumpsUsed = 0xffffffffu;
    gNdsFighterInitP0HitStatus = 0xffffffffu;
    gNdsFighterInitP1HitStatus = 0xffffffffu;
    gNdsFighterInitP0DamageKind = 0xffffffffu;
    gNdsFighterInitP1DamageKind = 0xffffffffu;
    gNdsFighterInitP0MotionAttackID = 0;
    gNdsFighterInitP1MotionAttackID = 0;
    gNdsFighterInitP0FloorProjectAttempt = 0;
    gNdsFighterInitP1FloorProjectAttempt = 0;
    gNdsFighterInitP0FloorProjectResult = 0;
    gNdsFighterInitP1FloorProjectResult = 0;
    gNdsFighterInitP0FloorLineID = 0xffffffffu;
    gNdsFighterInitP1FloorLineID = 0xffffffffu;
    gNdsFighterInitP0FloorDistBits = 0;
    gNdsFighterInitP1FloorDistBits = 0;
    gNdsFighterInitP0RootTranslateXBits = 0;
    gNdsFighterInitP1RootTranslateXBits = 0;
    gNdsFighterInitP0RootTranslateYBits = 0;
    gNdsFighterInitP1RootTranslateYBits = 0;
    gNdsFighterInitP0RootScaleXBits = 0;
    gNdsFighterInitP1RootScaleXBits = 0;
    gNdsFighterInitDamageCollMask = 0;
    gNdsFighterInitDamageCollNormalMask = 0;
    gNdsFighterInitDamageCollJointMask = 0;
    gNdsFighterInitDamageCollHalfSizeMask = 0;
    gNdsFighterInitDamageCollPartsMask = 0;
    gNdsFighterInitDamageCollMatrixMask = 0;
    gNdsFighterInitDamageCollScaleMask = 0;
    gNdsFighterInitP0DamageCollCount = 0;
    gNdsFighterInitP1DamageCollCount = 0;
    gNdsFighterInitP0DamageCollJoint0 = 0xffffffffu;
    gNdsFighterInitP1DamageCollJoint0 = 0xffffffffu;
    gNdsFighterInitP0DamageCollSizeXBits = 0;
    gNdsFighterInitP1DamageCollSizeXBits = 0;
    gNdsFighterInitP0DamageCollSizeYBits = 0;
    gNdsFighterInitP1DamageCollSizeYBits = 0;
    gNdsFighterInitP0DamageCollSizeZBits = 0;
    gNdsFighterInitP1DamageCollSizeZBits = 0;
    gNdsFighterInitP0DamageCollWorldXBits = 0;
    gNdsFighterInitP1DamageCollWorldXBits = 0;
    gNdsFighterInitP0DamageCollWorldYBits = 0;
    gNdsFighterInitP1DamageCollWorldYBits = 0;
    gNdsFighterInitP0DamageCollScaleXBits = 0;
    gNdsFighterInitP1DamageCollScaleXBits = 0;
    gNdsFighterInitPhysicsStopCount = 0;
    gNdsFighterInitAttackClearCount = 0;
    gNdsFighterInitHitStatusPartCount = 0;
    gNdsFighterInitColAnimResetCount = 0;
    gNdsFighterInitP0PassiveMarioTornado = 0xffffffffu;
    gNdsFighterInitP1PassiveMarioTornado = 0xffffffffu;
    gNdsFighterInitP0ThrowCatchItemClear = 0;
    gNdsFighterInitP1ThrowCatchItemClear = 0;
    gNdsFighterInitProcessAttachCount = 0;
    gNdsFighterInitStatusSetCount = 0;
    gNdsFighterInitDisplayProbeCount = 0;
    gNdsFighterMarioFoxWaitStatusResult = 0;
    gNdsFighterMarioFoxWaitMotionResult = 0;
    gNdsFighterMarioFoxWaitDeferResult = 0;
    gNdsFighterMarioFoxWaitMask = 0;
    gNdsFighterMarioFoxWaitDeferredMask = 0;
    gNdsFighterMarioFoxWaitCount = 0;
    gNdsFighterWaitP0StatusPrev = 0xffffffffu;
    gNdsFighterWaitP1StatusPrev = 0xffffffffu;
    gNdsFighterWaitP0StatusID = 0xffffffffu;
    gNdsFighterWaitP1StatusID = 0xffffffffu;
    gNdsFighterWaitP0MotionID = 0xffffffffu;
    gNdsFighterWaitP1MotionID = 0xffffffffu;
    gNdsFighterWaitP0MotionAttackID = 0xffffffffu;
    gNdsFighterWaitP1MotionAttackID = 0xffffffffu;
    gNdsFighterWaitP0StatusAttackID = 0xffffffffu;
    gNdsFighterWaitP1StatusAttackID = 0xffffffffu;
    gNdsFighterWaitP0AnimFrameBits = 0;
    gNdsFighterWaitP1AnimFrameBits = 0;
    gNdsFighterWaitP0AnimSpeedBits = 0;
    gNdsFighterWaitP1AnimSpeedBits = 0;
    gNdsFighterWaitP0SpecialInterrupt = 0;
    gNdsFighterWaitP1SpecialInterrupt = 0;
    gNdsFighterWaitP0PlayerTagWait = 0;
    gNdsFighterWaitP1PlayerTagWait = 0;
    gNdsFighterWaitP0ProcInterruptReady = 0;
    gNdsFighterWaitP1ProcInterruptReady = 0;
    gNdsFighterWaitP0ProcPhysicsReady = 0;
    gNdsFighterWaitP1ProcPhysicsReady = 0;
    gNdsFighterWaitP0ProcMapReady = 0;
    gNdsFighterWaitP1ProcMapReady = 0;
    gNdsFighterWaitP0MainMotionReady = 0;
    gNdsFighterWaitP1MainMotionReady = 0;
    gNdsFighterWaitP0GA = 0xffffffffu;
    gNdsFighterWaitP1GA = 0xffffffffu;
    gNdsFighterWaitFtMainSetStatusCallCount = 0;
    gNdsFighterWaitOriginalSetStatusCallCount = 0;
    gNdsFighterWaitHammerCheckCount = 0;
    gNdsFighterWaitHammerDeniedCount = 0;
    gNdsFighterWaitGroundSetCount = 0;
    gNdsFighterWaitPlayerTagSetCount = 0;
    gNdsFighterWaitProcInterruptCallCount = 0;
    gNdsFighterWaitProcPhysicsCallCount = 0;
    gNdsFighterWaitProcMapCallCount = 0;
    gNdsFighterWaitProcessAttachCount = 0;
    gNdsFighterWaitDisplayProbeCount = 0;
    gNdsFighterWaitGameplayUpdateCount = 0;
    gNdsFighterMarioFoxWaitTickResult = 0;
    gNdsFighterMarioFoxWaitCallbackResult = 0;
    gNdsFighterMarioFoxWaitSafeResult = 0;
    gNdsFighterMarioFoxWaitTickMask = 0;
    gNdsFighterMarioFoxWaitTickDeferredMask = 0;
    gNdsFighterMarioFoxWaitTickCount = 0;
    gNdsFighterWaitTickP0StatusBefore = 0xffffffffu;
    gNdsFighterWaitTickP1StatusBefore = 0xffffffffu;
    gNdsFighterWaitTickP0StatusAfter = 0xffffffffu;
    gNdsFighterWaitTickP1StatusAfter = 0xffffffffu;
    gNdsFighterWaitTickP0MotionBefore = 0xffffffffu;
    gNdsFighterWaitTickP1MotionBefore = 0xffffffffu;
    gNdsFighterWaitTickP0MotionAfter = 0xffffffffu;
    gNdsFighterWaitTickP1MotionAfter = 0xffffffffu;
    gNdsFighterWaitTickP0GABefore = 0xffffffffu;
    gNdsFighterWaitTickP1GABefore = 0xffffffffu;
    gNdsFighterWaitTickP0GAAfter = 0xffffffffu;
    gNdsFighterWaitTickP1GAAfter = 0xffffffffu;
    gNdsFighterWaitTickP0RootXBeforeBits = 0;
    gNdsFighterWaitTickP1RootXBeforeBits = 0;
    gNdsFighterWaitTickP0RootYBeforeBits = 0;
    gNdsFighterWaitTickP1RootYBeforeBits = 0;
    gNdsFighterWaitTickP0RootXAfterBits = 0;
    gNdsFighterWaitTickP1RootXAfterBits = 0;
    gNdsFighterWaitTickP0RootYAfterBits = 0;
    gNdsFighterWaitTickP1RootYAfterBits = 0;
    gNdsFighterWaitTickP0VelGroundXBeforeBits = 0;
    gNdsFighterWaitTickP1VelGroundXBeforeBits = 0;
    gNdsFighterWaitTickP0VelGroundXAfterBits = 0;
    gNdsFighterWaitTickP1VelGroundXAfterBits = 0;
    gNdsFighterWaitTickGObjCountBefore = 0;
    gNdsFighterWaitTickGObjCountAfter = 0;
    gNdsFighterWaitTickStatusChangeCount = 0;
    gNdsFighterWaitTickMotionChangeCount = 0;
    gNdsFighterWaitTickGADriftCount = 0;
    gNdsFighterWaitTickRootDriftCount = 0;
    gNdsFighterWaitTickGObjDelta = 0;
    gNdsFighterWaitTickOriginalInterruptCount = 0;
    gNdsFighterWaitTickGroundInterruptCheckCount = 0;
    gNdsFighterWaitTickPhysicsCallbackCount = 0;
    gNdsFighterWaitTickMapCallbackCount = 0;
    gNdsFighterWaitTickDeniedStatusCount = 0;
    gNdsFighterWaitTickProcessAttachCount = 0;
    gNdsFighterWaitTickDisplayProbeCount = 0;
    gNdsFighterWaitTickGameplayUpdateCount = 0;
    gNdsFighterMarioFoxGroundPhysResult = 0;
    gNdsFighterMarioFoxGroundMapResult = 0;
    gNdsFighterMarioFoxGroundSafeResult = 0;
    gNdsFighterMarioFoxGroundMask = 0;
    gNdsFighterMarioFoxGroundDeferredMask = 0;
    gNdsFighterMarioFoxGroundCount = 0;
    gNdsFighterWaitGroundP0VelBeforeMilli = 0;
    gNdsFighterWaitGroundP1VelBeforeMilli = 0;
    gNdsFighterWaitGroundP0VelAfterMilli = 0;
    gNdsFighterWaitGroundP1VelAfterMilli = 0;
    gNdsFighterWaitGroundP0AirVelXMilli = 0;
    gNdsFighterWaitGroundP1AirVelXMilli = 0;
    gNdsFighterWaitGroundP0AirVelYMilli = 0;
    gNdsFighterWaitGroundP1AirVelYMilli = 0;
    gNdsFighterWaitGroundP0FrictionMilli = 0;
    gNdsFighterWaitGroundP1FrictionMilli = 0;
    gNdsFighterWaitGroundP0Material = 0;
    gNdsFighterWaitGroundP1Material = 0;
    gNdsFighterWaitGroundP0TractionMilli = 0;
    gNdsFighterWaitGroundP1TractionMilli = 0;
    gNdsFighterWaitGroundP0StatusAfter = 0xffffffffu;
    gNdsFighterWaitGroundP1StatusAfter = 0xffffffffu;
    gNdsFighterWaitGroundP0MotionAfter = 0xffffffffu;
    gNdsFighterWaitGroundP1MotionAfter = 0xffffffffu;
    gNdsFighterWaitGroundP0GAAfter = 0xffffffffu;
    gNdsFighterWaitGroundP1GAAfter = 0xffffffffu;
    gNdsFighterWaitGroundP0RootXBeforeBits = 0;
    gNdsFighterWaitGroundP1RootXBeforeBits = 0;
    gNdsFighterWaitGroundP0RootXAfterBits = 0;
    gNdsFighterWaitGroundP1RootXAfterBits = 0;
    gNdsFighterWaitGroundP0RootYBeforeBits = 0;
    gNdsFighterWaitGroundP1RootYBeforeBits = 0;
    gNdsFighterWaitGroundP0RootYAfterBits = 0;
    gNdsFighterWaitGroundP1RootYAfterBits = 0;
    gNdsFighterWaitGroundPhysicsCallbackCount = 0;
    gNdsFighterWaitGroundMapCallbackCount = 0;
    gNdsFighterWaitGroundMapCheckCount = 0;
    gNdsFighterWaitGroundMapSafeFloorCount = 0;
    gNdsFighterWaitGroundMapFallDeniedCount = 0;
    gNdsFighterWaitGroundMapOttottoDeniedCount = 0;
    gNdsFighterWaitGroundStatusChangeCount = 0;
    gNdsFighterWaitGroundMotionChangeCount = 0;
    gNdsFighterWaitGroundGADriftCount = 0;
    gNdsFighterWaitGroundRootDriftCount = 0;
    gNdsFighterWaitGroundGObjDelta = 0;
    gNdsFighterWaitGroundDisplayProbeCount = 0;
    gNdsFighterWaitGroundGameplayUpdateCount = 0;
    gNdsFighterMarioFoxDisplayResult = 0;
    gNdsFighterMarioFoxDisplaySafeResult = 0;
    gNdsFighterMarioFoxDisplayMask = 0;
    gNdsFighterMarioFoxDisplayDeferredMask = 0;
    gNdsFighterMarioFoxDisplayCallbackCount = 0;
    gNdsFighterDisplayP0DObjCount = 0;
    gNdsFighterDisplayP1DObjCount = 0;
    gNdsFighterDisplayP0MObjCount = 0;
    gNdsFighterDisplayP1MObjCount = 0;
    gNdsFighterDisplayP0AObjCount = 0;
    gNdsFighterDisplayP1AObjCount = 0;
    gNdsFighterDisplayP0DLReadyCount = 0;
    gNdsFighterDisplayP1DLReadyCount = 0;
    gNdsFighterDisplayP0PartsPtrCount = 0;
    gNdsFighterDisplayP1PartsPtrCount = 0;
    gNdsFighterDisplayP0StatusAfter = 0xffffffffu;
    gNdsFighterDisplayP1StatusAfter = 0xffffffffu;
    gNdsFighterDisplayP0MotionAfter = 0xffffffffu;
    gNdsFighterDisplayP1MotionAfter = 0xffffffffu;
    gNdsFighterDisplayP0GAAfter = 0xffffffffu;
    gNdsFighterDisplayP1GAAfter = 0xffffffffu;
    gNdsFighterDisplayP0RootXBeforeBits = 0;
    gNdsFighterDisplayP1RootXBeforeBits = 0;
    gNdsFighterDisplayP0RootXAfterBits = 0;
    gNdsFighterDisplayP1RootXAfterBits = 0;
    gNdsFighterDisplayGObjDelta = 0;
    gNdsFighterDisplayDrawCallCount = 0;
    gNdsFighterDisplayMatrixCallCount = 0;
    gNdsFighterDisplayGameplayUpdateCount = 0;
    gNdsFighterMarioFoxDLScanResult = 0;
    gNdsFighterMarioFoxDLScanSafeResult = 0;
    gNdsFighterMarioFoxDLScanMask = 0;
    gNdsFighterMarioFoxDLScanDeferredMask = 0;
    gNdsFighterMarioFoxDLScanCount = 0;
    gNdsFighterDLScanP0FirstDL = 0;
    gNdsFighterDLScanP1FirstDL = 0;
    gNdsFighterDLScanP0AssetID = 0xffffffffu;
    gNdsFighterDLScanP1AssetID = 0xffffffffu;
    gNdsFighterDLScanP0Offset = 0;
    gNdsFighterDLScanP1Offset = 0;
    gNdsFighterDLScanP0DObjIndex = 0xffffffffu;
    gNdsFighterDLScanP1DObjIndex = 0xffffffffu;
    gNdsFighterDLScanP0Blocker = 0;
    gNdsFighterDLScanP1Blocker = 0;
    gNdsFighterDLScanP0CommandCount = 0;
    gNdsFighterDLScanP1CommandCount = 0;
    gNdsFighterDLScanP0FirstOpcode = 0;
    gNdsFighterDLScanP1FirstOpcode = 0;
    gNdsFighterDLScanP0UnsupportedOpcode = 0;
    gNdsFighterDLScanP1UnsupportedOpcode = 0;
    gNdsFighterDLScanP0UnsupportedCommandCount = 0;
    gNdsFighterDLScanP1UnsupportedCommandCount = 0;
    gNdsFighterDLScanP0VertexCommandCount = 0;
    gNdsFighterDLScanP1VertexCommandCount = 0;
    gNdsFighterDLScanP0TriangleCommandCount = 0;
    gNdsFighterDLScanP1TriangleCommandCount = 0;
    gNdsFighterDLScanP0VertexCount = 0;
    gNdsFighterDLScanP1VertexCount = 0;
    gNdsFighterDLScanP0TriangleCount = 0;
    gNdsFighterDLScanP1TriangleCount = 0;
    gNdsFighterDLScanP0EndCommandCount = 0;
    gNdsFighterDLScanP1EndCommandCount = 0;
    gNdsFighterDLScanP0BranchCommandCount = 0;
    gNdsFighterDLScanP1BranchCommandCount = 0;
    gNdsFighterDLScanP0SegmentResolveCount = 0;
    gNdsFighterDLScanP1SegmentResolveCount = 0;
    gNdsFighterDLScanP0TextureMask = 0;
    gNdsFighterDLScanP1TextureMask = 0;
    gNdsFighterDLScanP0OtherModeCommandCount = 0;
    gNdsFighterDLScanP1OtherModeCommandCount = 0;
    gNdsFighterDLScanP0CullCommandCount = 0;
    gNdsFighterDLScanP1CullCommandCount = 0;
    gNdsFighterDLScanP0StateCommandCount = 0;
    gNdsFighterDLScanP1StateCommandCount = 0;
    gNdsFighterDLScanP0SkipCommandCount = 0;
    gNdsFighterDLScanP1SkipCommandCount = 0;
    gNdsFighterDLScanP0RenderCommandCount = 0;
    gNdsFighterDLScanP1RenderCommandCount = 0;
    gNdsFighterDLScanP0MaxDepthSeen = 0;
    gNdsFighterDLScanP1MaxDepthSeen = 0;
    gNdsFighterDLScanP0StatusAfter = 0xffffffffu;
    gNdsFighterDLScanP1StatusAfter = 0xffffffffu;
    gNdsFighterDLScanP0MotionAfter = 0xffffffffu;
    gNdsFighterDLScanP1MotionAfter = 0xffffffffu;
    gNdsFighterDLScanP0GAAfter = 0xffffffffu;
    gNdsFighterDLScanP1GAAfter = 0xffffffffu;
    gNdsFighterDLScanP0RootXBeforeBits = 0;
    gNdsFighterDLScanP0RootXAfterBits = 0;
    gNdsFighterDLScanP1RootXBeforeBits = 0;
    gNdsFighterDLScanP1RootXAfterBits = 0;
    gNdsFighterDLScanGObjDelta = 0;
    gNdsFighterDLScanDrawCallCount = 0;
    gNdsFighterDLScanMatrixCallCount = 0;
    gNdsFighterDLScanGameplayUpdateCount = 0;
    gNdsFighterDLScanRangeRejectCount = 0;
    gNdsFighterDLScanBranchResolveCount = 0;
    gNdsFighterMarioFoxDLExecResult = 0;
    gNdsFighterMarioFoxDLExecSafeResult = 0;
    gNdsFighterMarioFoxDLExecMask = 0;
    gNdsFighterMarioFoxDLExecDeferredMask = 0;
    gNdsFighterMarioFoxDLExecCount = 0;
    gNdsFighterDLExecP0Blocker = 0;
    gNdsFighterDLExecP1Blocker = 0;
    gNdsFighterDLExecP0CommandCount = 0;
    gNdsFighterDLExecP1CommandCount = 0;
    gNdsFighterDLExecP0FirstOpcode = 0;
    gNdsFighterDLExecP1FirstOpcode = 0;
    gNdsFighterDLExecP0UnsupportedOpcode = 0;
    gNdsFighterDLExecP1UnsupportedOpcode = 0;
    gNdsFighterDLExecP0UnsupportedCommandCount = 0;
    gNdsFighterDLExecP1UnsupportedCommandCount = 0;
    gNdsFighterDLExecP0VertexCommandCount = 0;
    gNdsFighterDLExecP1VertexCommandCount = 0;
    gNdsFighterDLExecP0VertexDecodedCount = 0;
    gNdsFighterDLExecP1VertexDecodedCount = 0;
    gNdsFighterDLExecP0VertexValidMask = 0;
    gNdsFighterDLExecP1VertexValidMask = 0;
    gNdsFighterDLExecP0TriangleCommandCount = 0;
    gNdsFighterDLExecP1TriangleCommandCount = 0;
    gNdsFighterDLExecP0TriangleCount = 0;
    gNdsFighterDLExecP1TriangleCount = 0;
    gNdsFighterDLExecP0TriangleValidCount = 0;
    gNdsFighterDLExecP1TriangleValidCount = 0;
    gNdsFighterDLExecP0MinX = 0;
    gNdsFighterDLExecP0MaxX = 0;
    gNdsFighterDLExecP0MinY = 0;
    gNdsFighterDLExecP0MaxY = 0;
    gNdsFighterDLExecP0MinZ = 0;
    gNdsFighterDLExecP0MaxZ = 0;
    gNdsFighterDLExecP1MinX = 0;
    gNdsFighterDLExecP1MaxX = 0;
    gNdsFighterDLExecP1MinY = 0;
    gNdsFighterDLExecP1MaxY = 0;
    gNdsFighterDLExecP1MinZ = 0;
    gNdsFighterDLExecP1MaxZ = 0;
    gNdsFighterDLExecP0ColorChecksum = 0;
    gNdsFighterDLExecP1ColorChecksum = 0;
    gNdsFighterDLExecP0OtherModeCommandCount = 0;
    gNdsFighterDLExecP1OtherModeCommandCount = 0;
    gNdsFighterDLExecP0CullCommandCount = 0;
    gNdsFighterDLExecP1CullCommandCount = 0;
    gNdsFighterDLExecP0StateCommandCount = 0;
    gNdsFighterDLExecP1StateCommandCount = 0;
    gNdsFighterDLExecP0SkipCommandCount = 0;
    gNdsFighterDLExecP1SkipCommandCount = 0;
    gNdsFighterDLExecP0RenderCommandCount = 0;
    gNdsFighterDLExecP1RenderCommandCount = 0;
    gNdsFighterDLExecP0BranchCommandCount = 0;
    gNdsFighterDLExecP1BranchCommandCount = 0;
    gNdsFighterDLExecP0SegmentResolveCount = 0;
    gNdsFighterDLExecP1SegmentResolveCount = 0;
    gNdsFighterDLExecP0TextureMask = 0;
    gNdsFighterDLExecP1TextureMask = 0;
    gNdsFighterDLExecP0StatusAfter = 0xffffffffu;
    gNdsFighterDLExecP1StatusAfter = 0xffffffffu;
    gNdsFighterDLExecP0MotionAfter = 0xffffffffu;
    gNdsFighterDLExecP1MotionAfter = 0xffffffffu;
    gNdsFighterDLExecP0GAAfter = 0xffffffffu;
    gNdsFighterDLExecP1GAAfter = 0xffffffffu;
    gNdsFighterDLExecP0RootXBeforeBits = 0;
    gNdsFighterDLExecP0RootXAfterBits = 0;
    gNdsFighterDLExecP1RootXBeforeBits = 0;
    gNdsFighterDLExecP1RootXAfterBits = 0;
    gNdsFighterDLExecGObjDelta = 0;
    gNdsFighterDLExecDrawCallCount = 0;
    gNdsFighterDLExecMatrixCallCount = 0;
    gNdsFighterDLExecGameplayUpdateCount = 0;
    gNdsFighterDLExecRangeRejectCount = 0;
    gNdsFighterDLExecVertexRangeRejectCount = 0;
    gNdsFighterMarioFoxDLDrawResult = 0;
    gNdsFighterMarioFoxDLDrawSafeResult = 0;
    gNdsFighterMarioFoxDLDrawMask = 0;
    gNdsFighterMarioFoxDLDrawDeferredMask = 0;
    gNdsFighterMarioFoxDLDrawCount = 0;
    gNdsFighterDLDrawPreviewWidth = 0;
    gNdsFighterDLDrawPreviewHeight = 0;
    gNdsFighterDLDrawPreviewPitch = 0;
    gNdsFighterDLDrawPreviewReady = 0;
    gNdsFighterDLDrawPreviewCommitBefore = 0;
    gNdsFighterDLDrawPreviewCommitAfter = 0;
    gNdsFighterDLDrawPreviewCommitDelta = 0;
    gNdsFighterDLDrawP0Blocker = 0;
    gNdsFighterDLDrawP1Blocker = 0;
    gNdsFighterDLDrawP0CommandCount = 0;
    gNdsFighterDLDrawP1CommandCount = 0;
    gNdsFighterDLDrawP0FirstOpcode = 0;
    gNdsFighterDLDrawP1FirstOpcode = 0;
    gNdsFighterDLDrawP0UnsupportedOpcode = 0;
    gNdsFighterDLDrawP1UnsupportedOpcode = 0;
    gNdsFighterDLDrawP0UnsupportedCommandCount = 0;
    gNdsFighterDLDrawP1UnsupportedCommandCount = 0;
    gNdsFighterDLDrawP0VertexDecodedCount = 0;
    gNdsFighterDLDrawP1VertexDecodedCount = 0;
    gNdsFighterDLDrawP0TriangleCount = 0;
    gNdsFighterDLDrawP1TriangleCount = 0;
    gNdsFighterDLDrawP0TriangleValidCount = 0;
    gNdsFighterDLDrawP1TriangleValidCount = 0;
    gNdsFighterDLDrawP0TriangleDrawnCount = 0;
    gNdsFighterDLDrawP1TriangleDrawnCount = 0;
    gNdsFighterDLDrawP0RealTriangleDrawnCount = 0;
    gNdsFighterDLDrawP1RealTriangleDrawnCount = 0;
    gNdsFighterDLDrawP0MarkerTriangleDrawnCount = 0;
    gNdsFighterDLDrawP1MarkerTriangleDrawnCount = 0;
    gNdsFighterDLDrawP0PixelCount = 0;
    gNdsFighterDLDrawP1PixelCount = 0;
    gNdsFighterDLDrawTotalPixelCount = 0;
    gNdsFighterDLDrawP0Axis = 0xffffffffu;
    gNdsFighterDLDrawP1Axis = 0xffffffffu;
    gNdsFighterDLDrawP0Area = 0;
    gNdsFighterDLDrawP1Area = 0;
    gNdsFighterDLDrawP0MinA = 0;
    gNdsFighterDLDrawP0MaxA = 0;
    gNdsFighterDLDrawP0MinB = 0;
    gNdsFighterDLDrawP0MaxB = 0;
    gNdsFighterDLDrawP1MinA = 0;
    gNdsFighterDLDrawP1MaxA = 0;
    gNdsFighterDLDrawP1MinB = 0;
    gNdsFighterDLDrawP1MaxB = 0;
    gNdsFighterDLDrawP0ScreenMinX = 0;
    gNdsFighterDLDrawP0ScreenMaxX = 0;
    gNdsFighterDLDrawP0ScreenMinY = 0;
    gNdsFighterDLDrawP0ScreenMaxY = 0;
    gNdsFighterDLDrawP1ScreenMinX = 0;
    gNdsFighterDLDrawP1ScreenMaxX = 0;
    gNdsFighterDLDrawP1ScreenMinY = 0;
    gNdsFighterDLDrawP1ScreenMaxY = 0;
    gNdsFighterDLDrawP0ColorChecksum = 0;
    gNdsFighterDLDrawP1ColorChecksum = 0;
    gNdsFighterDLDrawP0StatusAfter = 0xffffffffu;
    gNdsFighterDLDrawP1StatusAfter = 0xffffffffu;
    gNdsFighterDLDrawP0MotionAfter = 0xffffffffu;
    gNdsFighterDLDrawP1MotionAfter = 0xffffffffu;
    gNdsFighterDLDrawP0GAAfter = 0xffffffffu;
    gNdsFighterDLDrawP1GAAfter = 0xffffffffu;
    gNdsFighterDLDrawP0RootXBeforeBits = 0;
    gNdsFighterDLDrawP0RootXAfterBits = 0;
    gNdsFighterDLDrawP1RootXBeforeBits = 0;
    gNdsFighterDLDrawP1RootXAfterBits = 0;
    gNdsFighterDLDrawGObjDelta = 0;
    gNdsFighterDLDrawDrawCallCount = 0;
    gNdsFighterDLDrawMatrixCallCount = 0;
    gNdsFighterDLDrawGameplayUpdateCount = 0;
    gNdsFighterDLDrawRangeRejectCount = 0;
    gNdsFighterDLDrawVertexRangeRejectCount = 0;
    gNdsFighterMarioFoxDLMultiDrawResult = 0;
    gNdsFighterMarioFoxDLMultiDrawSafeResult = 0;
    gNdsFighterMarioFoxDLMultiDrawMask = 0;
    gNdsFighterMarioFoxDLMultiDrawDeferredMask = 0;
    gNdsFighterMarioFoxDLMultiDrawCount = 0;
    gNdsFighterDLMultiDrawPreviewWidth = 0;
    gNdsFighterDLMultiDrawPreviewHeight = 0;
    gNdsFighterDLMultiDrawPreviewPitch = 0;
    gNdsFighterDLMultiDrawPreviewReady = 0;
    gNdsFighterDLMultiDrawPreviewCommitBefore = 0;
    gNdsFighterDLMultiDrawPreviewCommitAfter = 0;
    gNdsFighterDLMultiDrawPreviewCommitDelta = 0;
    gNdsFighterDLMultiDrawP0CandidateCount = 0;
    gNdsFighterDLMultiDrawP1CandidateCount = 0;
    gNdsFighterDLMultiDrawP0SelectedCount = 0;
    gNdsFighterDLMultiDrawP1SelectedCount = 0;
    gNdsFighterDLMultiDrawP0AttemptCount = 0;
    gNdsFighterDLMultiDrawP1AttemptCount = 0;
    gNdsFighterDLMultiDrawP0CleanCount = 0;
    gNdsFighterDLMultiDrawP1CleanCount = 0;
    gNdsFighterDLMultiDrawP0DrawnDObjCount = 0;
    gNdsFighterDLMultiDrawP1DrawnDObjCount = 0;
    gNdsFighterDLMultiDrawP0FailedCount = 0;
    gNdsFighterDLMultiDrawP1FailedCount = 0;
    gNdsFighterDLMultiDrawP0SelectedIndexMask = 0;
    gNdsFighterDLMultiDrawP1SelectedIndexMask = 0;
    gNdsFighterDLMultiDrawP0FirstBlocker = 0;
    gNdsFighterDLMultiDrawP1FirstBlocker = 0;
    gNdsFighterDLMultiDrawP0BlockerMask = 0;
    gNdsFighterDLMultiDrawP1BlockerMask = 0;
    gNdsFighterDLMultiDrawP0CommandCount = 0;
    gNdsFighterDLMultiDrawP1CommandCount = 0;
    gNdsFighterDLMultiDrawP0FirstOpcode = 0;
    gNdsFighterDLMultiDrawP1FirstOpcode = 0;
    gNdsFighterDLMultiDrawP0UnsupportedOpcode = 0;
    gNdsFighterDLMultiDrawP1UnsupportedOpcode = 0;
    gNdsFighterDLMultiDrawP0UnsupportedCommandCount = 0;
    gNdsFighterDLMultiDrawP1UnsupportedCommandCount = 0;
    gNdsFighterDLMultiDrawP0VertexDecodedCount = 0;
    gNdsFighterDLMultiDrawP1VertexDecodedCount = 0;
    gNdsFighterDLMultiDrawP0TriangleCount = 0;
    gNdsFighterDLMultiDrawP1TriangleCount = 0;
    gNdsFighterDLMultiDrawP0TriangleValidCount = 0;
    gNdsFighterDLMultiDrawP1TriangleValidCount = 0;
    gNdsFighterDLMultiDrawP0TriangleDrawnCount = 0;
    gNdsFighterDLMultiDrawP1TriangleDrawnCount = 0;
    gNdsFighterDLMultiDrawP0RealTriangleDrawnCount = 0;
    gNdsFighterDLMultiDrawP1RealTriangleDrawnCount = 0;
    gNdsFighterDLMultiDrawP0MarkerTriangleDrawnCount = 0;
    gNdsFighterDLMultiDrawP1MarkerTriangleDrawnCount = 0;
    gNdsFighterDLMultiDrawP0PixelCount = 0;
    gNdsFighterDLMultiDrawP1PixelCount = 0;
    gNdsFighterDLMultiDrawTotalPixelCount = 0;
    gNdsFighterDLMultiDrawP0Axis = 0xffffffffu;
    gNdsFighterDLMultiDrawP1Axis = 0xffffffffu;
    gNdsFighterDLMultiDrawP0Area = 0;
    gNdsFighterDLMultiDrawP1Area = 0;
    gNdsFighterDLMultiDrawP0MinA = 0;
    gNdsFighterDLMultiDrawP0MaxA = 0;
    gNdsFighterDLMultiDrawP0MinB = 0;
    gNdsFighterDLMultiDrawP0MaxB = 0;
    gNdsFighterDLMultiDrawP1MinA = 0;
    gNdsFighterDLMultiDrawP1MaxA = 0;
    gNdsFighterDLMultiDrawP1MinB = 0;
    gNdsFighterDLMultiDrawP1MaxB = 0;
    gNdsFighterDLMultiDrawP0ScreenMinX = 0;
    gNdsFighterDLMultiDrawP0ScreenMaxX = 0;
    gNdsFighterDLMultiDrawP0ScreenMinY = 0;
    gNdsFighterDLMultiDrawP0ScreenMaxY = 0;
    gNdsFighterDLMultiDrawP1ScreenMinX = 0;
    gNdsFighterDLMultiDrawP1ScreenMaxX = 0;
    gNdsFighterDLMultiDrawP1ScreenMinY = 0;
    gNdsFighterDLMultiDrawP1ScreenMaxY = 0;
    gNdsFighterDLMultiDrawP0ColorChecksum = 0;
    gNdsFighterDLMultiDrawP1ColorChecksum = 0;
    gNdsFighterDLMultiDrawP0StatusAfter = 0xffffffffu;
    gNdsFighterDLMultiDrawP1StatusAfter = 0xffffffffu;
    gNdsFighterDLMultiDrawP0MotionAfter = 0xffffffffu;
    gNdsFighterDLMultiDrawP1MotionAfter = 0xffffffffu;
    gNdsFighterDLMultiDrawP0GAAfter = 0xffffffffu;
    gNdsFighterDLMultiDrawP1GAAfter = 0xffffffffu;
    gNdsFighterDLMultiDrawP0RootXBeforeBits = 0;
    gNdsFighterDLMultiDrawP0RootXAfterBits = 0;
    gNdsFighterDLMultiDrawP1RootXBeforeBits = 0;
    gNdsFighterDLMultiDrawP1RootXAfterBits = 0;
    gNdsFighterDLMultiDrawGObjDelta = 0;
    gNdsFighterDLMultiDrawDrawCallCount = 0;
    gNdsFighterDLMultiDrawMatrixCallCount = 0;
    gNdsFighterDLMultiDrawGameplayUpdateCount = 0;
    gNdsFighterDLMultiDrawRangeRejectCount = 0;
    gNdsFighterDLMultiDrawVertexRangeRejectCount = 0;
    gNdsFighterMarioFoxDLAllDrawResult = 0;
    gNdsFighterMarioFoxDLAllDrawSafeResult = 0;
    gNdsFighterMarioFoxDLAllDrawMask = 0;
    gNdsFighterMarioFoxDLAllDrawDeferredMask = 0;
    gNdsFighterMarioFoxDLAllDrawCount = 0;
    gNdsFighterDLAllDrawDisplayCallbackCount = 0;
    gNdsFighterDLAllDrawP0DisplayCallbackCount = 0;
    gNdsFighterDLAllDrawP1DisplayCallbackCount = 0;
    gNdsFighterDLAllDrawPreviewWidth = 0;
    gNdsFighterDLAllDrawPreviewHeight = 0;
    gNdsFighterDLAllDrawPreviewPitch = 0;
    gNdsFighterDLAllDrawPreviewReady = 0;
    gNdsFighterDLAllDrawPreviewCommitBefore = 0;
    gNdsFighterDLAllDrawPreviewCommitAfter = 0;
    gNdsFighterDLAllDrawPreviewCommitDelta = 0;
    gNdsFighterDLAllDrawP0CandidateCount = 0;
    gNdsFighterDLAllDrawP1CandidateCount = 0;
    gNdsFighterDLAllDrawP0SelectedCount = 0;
    gNdsFighterDLAllDrawP1SelectedCount = 0;
    gNdsFighterDLAllDrawP0AttemptCount = 0;
    gNdsFighterDLAllDrawP1AttemptCount = 0;
    gNdsFighterDLAllDrawP0CleanCount = 0;
    gNdsFighterDLAllDrawP1CleanCount = 0;
    gNdsFighterDLAllDrawP0DrawnDObjCount = 0;
    gNdsFighterDLAllDrawP1DrawnDObjCount = 0;
    gNdsFighterDLAllDrawP0FailedCount = 0;
    gNdsFighterDLAllDrawP1FailedCount = 0;
    gNdsFighterDLAllDrawP0SelectedIndexMask = 0;
    gNdsFighterDLAllDrawP1SelectedIndexMask = 0;
    gNdsFighterDLAllDrawP0FirstBlocker = 0;
    gNdsFighterDLAllDrawP1FirstBlocker = 0;
    gNdsFighterDLAllDrawP0BlockerMask = 0;
    gNdsFighterDLAllDrawP1BlockerMask = 0;
    gNdsFighterDLAllDrawP0CommandCount = 0;
    gNdsFighterDLAllDrawP1CommandCount = 0;
    gNdsFighterDLAllDrawP0FirstOpcode = 0;
    gNdsFighterDLAllDrawP1FirstOpcode = 0;
    gNdsFighterDLAllDrawP0UnsupportedOpcode = 0;
    gNdsFighterDLAllDrawP1UnsupportedOpcode = 0;
    gNdsFighterDLAllDrawP0UnsupportedCommandCount = 0;
    gNdsFighterDLAllDrawP1UnsupportedCommandCount = 0;
    gNdsFighterDLAllDrawP0VertexDecodedCount = 0;
    gNdsFighterDLAllDrawP1VertexDecodedCount = 0;
    gNdsFighterDLAllDrawP0MatrixMvpRecalcCount = 0;
    gNdsFighterDLAllDrawP1MatrixMvpRecalcCount = 0;
    gNdsFighterDLAllDrawP0MatrixMoveWordCount = 0;
    gNdsFighterDLAllDrawP1MatrixMoveWordCount = 0;
    gNdsFighterDLAllDrawP0HardwareTriangleCount = 0;
    gNdsFighterDLAllDrawP1HardwareTriangleCount = 0;
    gNdsFighterDLAllDrawP0HardwareOracleTriangleCount = 0;
    gNdsFighterDLAllDrawP1HardwareOracleTriangleCount = 0;
    gNdsFighterDLAllDrawP0HardwareOracleRejectCount = 0;
    gNdsFighterDLAllDrawP1HardwareOracleRejectCount = 0;
    gNdsFighterDLAllDrawP0HardwareMatrixSeedCount = 0;
    gNdsFighterDLAllDrawP1HardwareMatrixSeedCount = 0;
    gNdsFighterDLAllDrawHardwareTextureBindCount = 0;
    gNdsFighterDLAllDrawHardwareTextureUploadCount = 0;
    gNdsFighterDLAllDrawHardwareTextureReadyCount = 0;
    gNdsFighterDLAllDrawHardwareTextureRejectCount = 0;
    gNdsFighterDLAllDrawHardwareTextureFormatMask = 0;
    gNdsFighterDLAllDrawHardwareTextureMaxWidth = 0;
    gNdsFighterDLAllDrawHardwareTextureMaxHeight = 0;
    gNdsFighterDLAllDrawP0TriangleCount = 0;
    gNdsFighterDLAllDrawP1TriangleCount = 0;
    gNdsFighterDLAllDrawP0TriangleValidCount = 0;
    gNdsFighterDLAllDrawP1TriangleValidCount = 0;
    gNdsFighterDLAllDrawP0TriangleDrawnCount = 0;
    gNdsFighterDLAllDrawP1TriangleDrawnCount = 0;
    gNdsFighterDLAllDrawP0RealTriangleDrawnCount = 0;
    gNdsFighterDLAllDrawP1RealTriangleDrawnCount = 0;
    gNdsFighterDLAllDrawP0MarkerTriangleDrawnCount = 0;
    gNdsFighterDLAllDrawP1MarkerTriangleDrawnCount = 0;
    gNdsFighterDLAllDrawP0PixelCount = 0;
    gNdsFighterDLAllDrawP1PixelCount = 0;
    gNdsFighterDLAllDrawTotalPixelCount = 0;
    gNdsFighterDLAllDrawP0Axis = 0xffffffffu;
    gNdsFighterDLAllDrawP1Axis = 0xffffffffu;
    gNdsFighterDLAllDrawP0Area = 0;
    gNdsFighterDLAllDrawP1Area = 0;
    gNdsFighterDLAllDrawP0MinA = 0;
    gNdsFighterDLAllDrawP0MaxA = 0;
    gNdsFighterDLAllDrawP0MinB = 0;
    gNdsFighterDLAllDrawP0MaxB = 0;
    gNdsFighterDLAllDrawP1MinA = 0;
    gNdsFighterDLAllDrawP1MaxA = 0;
    gNdsFighterDLAllDrawP1MinB = 0;
    gNdsFighterDLAllDrawP1MaxB = 0;
    gNdsFighterDLAllDrawP0ScreenMinX = 0;
    gNdsFighterDLAllDrawP0ScreenMaxX = 0;
    gNdsFighterDLAllDrawP0ScreenMinY = 0;
    gNdsFighterDLAllDrawP0ScreenMaxY = 0;
    gNdsFighterDLAllDrawP1ScreenMinX = 0;
    gNdsFighterDLAllDrawP1ScreenMaxX = 0;
    gNdsFighterDLAllDrawP1ScreenMinY = 0;
    gNdsFighterDLAllDrawP1ScreenMaxY = 0;
    gNdsFighterDLAllDrawP0ColorChecksum = 0;
    gNdsFighterDLAllDrawP1ColorChecksum = 0;
    gNdsFighterDLAllDrawP0StatusAfter = 0xffffffffu;
    gNdsFighterDLAllDrawP1StatusAfter = 0xffffffffu;
    gNdsFighterDLAllDrawP0MotionAfter = 0xffffffffu;
    gNdsFighterDLAllDrawP1MotionAfter = 0xffffffffu;
    gNdsFighterDLAllDrawP0GAAfter = 0xffffffffu;
    gNdsFighterDLAllDrawP1GAAfter = 0xffffffffu;
    gNdsFighterDLAllDrawP0RootXBeforeBits = 0;
    gNdsFighterDLAllDrawP0RootXAfterBits = 0;
    gNdsFighterDLAllDrawP1RootXBeforeBits = 0;
    gNdsFighterDLAllDrawP1RootXAfterBits = 0;
    gNdsFighterDLAllDrawGObjDelta = 0;
    gNdsFighterDLAllDrawDrawCallCount = 0;
    gNdsFighterDLAllDrawMatrixCallCount = 0;
    gNdsFighterDLAllDrawGameplayUpdateCount = 0;
    gNdsFighterDLAllDrawRangeRejectCount = 0;
    gNdsFighterDLAllDrawVertexRangeRejectCount = 0;
    gNdsFighterMarioFoxWalkInputResult = 0;
    gNdsFighterMarioFoxWalkSafeResult = 0;
    gNdsFighterMarioFoxWalkInputMask = 0;
    gNdsFighterMarioFoxWalkDeferredMask = 0;
    gNdsFighterMarioFoxWalkInputCount = 0;
    gNdsFighterWalkP0StickX = 0;
    gNdsFighterWalkP1StickX = 0;
    gNdsFighterWalkP0StickAbs = 0;
    gNdsFighterWalkP1StickAbs = 0;
    gNdsFighterWalkP0LR = 0;
    gNdsFighterWalkP1LR = 0;
    gNdsFighterWalkP0InputSuccess = 0;
    gNdsFighterWalkP1InputSuccess = 0;
    gNdsFighterWalkP0SelectedStatus = 0;
    gNdsFighterWalkP1SelectedStatus = 0;
    gNdsFighterWalkP0StatusBefore = 0xffffffffu;
    gNdsFighterWalkP1StatusBefore = 0xffffffffu;
    gNdsFighterWalkP0StatusAfter = 0xffffffffu;
    gNdsFighterWalkP1StatusAfter = 0xffffffffu;
    gNdsFighterWalkP0MotionBefore = 0xffffffffu;
    gNdsFighterWalkP1MotionBefore = 0xffffffffu;
    gNdsFighterWalkP0MotionAfter = 0xffffffffu;
    gNdsFighterWalkP1MotionAfter = 0xffffffffu;
    gNdsFighterWalkP0GABefore = 0xffffffffu;
    gNdsFighterWalkP1GABefore = 0xffffffffu;
    gNdsFighterWalkP0GAAfter = 0xffffffffu;
    gNdsFighterWalkP1GAAfter = 0xffffffffu;
    gNdsFighterWalkWaitInterruptCallCount = 0;
    gNdsFighterWalkGroundCheckCallCount = 0;
    gNdsFighterWalkOriginalCheckCallCount = 0;
    gNdsFighterWalkOriginalCheckSuccessCount = 0;
    gNdsFighterWalkSetStatusCallCount = 0;
    gNdsFighterWalkFtMainSetStatusCallCount = 0;
    gNdsFighterWalkAnimEventsCallCount = 0;
    gNdsFighterWalkCallbackReadyCount = 0;
    gNdsFighterWalkLoopInterruptCallCount = 0;
    gNdsFighterWalkDeferredInterruptCheckCount = 0;
    gNdsFighterWalkP0GroundVelBeforeMilli = 0;
    gNdsFighterWalkP1GroundVelBeforeMilli = 0;
    gNdsFighterWalkP0GroundVelAfterMilli = 0;
    gNdsFighterWalkP1GroundVelAfterMilli = 0;
    gNdsFighterWalkP0AirVelXMilli = 0;
    gNdsFighterWalkP1AirVelXMilli = 0;
    gNdsFighterWalkP0AirVelYMilli = 0;
    gNdsFighterWalkP1AirVelYMilli = 0;
    gNdsFighterWalkGroundVelAbsStickCount = 0;
    gNdsFighterWalkGroundVelTransferAirCount = 0;
    gNdsFighterWalkPhysicsCallbackCount = 0;
    gNdsFighterWalkMapCallbackCount = 0;
    gNdsFighterWalkMapSafeFloorCount = 0;
    gNdsFighterWalkMapFallDeniedCount = 0;
    gNdsFighterWalkMapOttottoDeniedCount = 0;
    gNdsFighterWalkP0RootXBeforeBits = 0;
    gNdsFighterWalkP0RootXAfterBits = 0;
    gNdsFighterWalkP0RootYBeforeBits = 0;
    gNdsFighterWalkP0RootYAfterBits = 0;
    gNdsFighterWalkP1RootXBeforeBits = 0;
    gNdsFighterWalkP1RootXAfterBits = 0;
    gNdsFighterWalkP1RootYBeforeBits = 0;
    gNdsFighterWalkP1RootYAfterBits = 0;
    gNdsFighterWalkGObjDelta = 0;
    gNdsFighterWalkDeniedStatusCount = 0;
    gNdsFighterWalkUnexpectedStatusCount = 0;
    gNdsFighterWalkProcessAttachCount = 0;
    gNdsFighterWalkDisplayProbeCount = 0;
    gNdsFighterWalkGameplayUpdateCount = 0;
    gNdsFighterWalkDrawCallCount = 0;
    gNdsFighterWalkMatrixCallCount = 0;
    gNdsFighterMarioFoxWalkLoopResult = 0;
    gNdsFighterMarioFoxWalkLoopSafeResult = 0;
    gNdsFighterMarioFoxWalkLoopMask = 0;
    gNdsFighterMarioFoxWalkLoopDeferredMask = 0;
    gNdsFighterMarioFoxWalkLoopCount = 0;
    gNdsFighterWalkLoopFrameTarget = 0;
    gNdsFighterWalkLoopP0HeldFrameCount = 0;
    gNdsFighterWalkLoopP1HeldFrameCount = 0;
    gNdsFighterWalkLoopP0InterruptCount = 0;
    gNdsFighterWalkLoopP1InterruptCount = 0;
    gNdsFighterWalkLoopP0PhysicsCount = 0;
    gNdsFighterWalkLoopP1PhysicsCount = 0;
    gNdsFighterWalkLoopP0IntegrateCount = 0;
    gNdsFighterWalkLoopP1IntegrateCount = 0;
    gNdsFighterWalkLoopP0MapCount = 0;
    gNdsFighterWalkLoopP1MapCount = 0;
    gNdsFighterWalkLoopP0SafeFloorCount = 0;
    gNdsFighterWalkLoopP1SafeFloorCount = 0;
    gNdsFighterWalkLoopP0StickX = 0;
    gNdsFighterWalkLoopP1StickX = 0;
    gNdsFighterWalkLoopP0StickAbs = 0;
    gNdsFighterWalkLoopP1StickAbs = 0;
    gNdsFighterWalkLoopP0LR = 0;
    gNdsFighterWalkLoopP1LR = 0;
    gNdsFighterWalkLoopP0StatusStart = 0xffffffffu;
    gNdsFighterWalkLoopP1StatusStart = 0xffffffffu;
    gNdsFighterWalkLoopP0StatusAfterHeld = 0xffffffffu;
    gNdsFighterWalkLoopP1StatusAfterHeld = 0xffffffffu;
    gNdsFighterWalkLoopP0StatusAfterRelease = 0xffffffffu;
    gNdsFighterWalkLoopP1StatusAfterRelease = 0xffffffffu;
    gNdsFighterWalkLoopP0StatusAfterSettle = 0xffffffffu;
    gNdsFighterWalkLoopP1StatusAfterSettle = 0xffffffffu;
    gNdsFighterWalkLoopP0MotionStart = 0xffffffffu;
    gNdsFighterWalkLoopP1MotionStart = 0xffffffffu;
    gNdsFighterWalkLoopP0MotionAfterHeld = 0xffffffffu;
    gNdsFighterWalkLoopP1MotionAfterHeld = 0xffffffffu;
    gNdsFighterWalkLoopP0MotionAfterRelease = 0xffffffffu;
    gNdsFighterWalkLoopP1MotionAfterRelease = 0xffffffffu;
    gNdsFighterWalkLoopP0MotionAfterSettle = 0xffffffffu;
    gNdsFighterWalkLoopP1MotionAfterSettle = 0xffffffffu;
    gNdsFighterWalkLoopP0GAStart = 0xffffffffu;
    gNdsFighterWalkLoopP1GAStart = 0xffffffffu;
    gNdsFighterWalkLoopP0GAAfterHeld = 0xffffffffu;
    gNdsFighterWalkLoopP1GAAfterHeld = 0xffffffffu;
    gNdsFighterWalkLoopP0GAAfterRelease = 0xffffffffu;
    gNdsFighterWalkLoopP1GAAfterRelease = 0xffffffffu;
    gNdsFighterWalkLoopP0GAAfterSettle = 0xffffffffu;
    gNdsFighterWalkLoopP1GAAfterSettle = 0xffffffffu;
    gNdsFighterWalkLoopP0RootXStartBits = 0;
    gNdsFighterWalkLoopP0RootXAfterHeldBits = 0;
    gNdsFighterWalkLoopP0RootXAfterSettleBits = 0;
    gNdsFighterWalkLoopP1RootXStartBits = 0;
    gNdsFighterWalkLoopP1RootXAfterHeldBits = 0;
    gNdsFighterWalkLoopP1RootXAfterSettleBits = 0;
    gNdsFighterWalkLoopP0RootYStartBits = 0;
    gNdsFighterWalkLoopP0RootYAfterHeldBits = 0;
    gNdsFighterWalkLoopP0RootYAfterSettleBits = 0;
    gNdsFighterWalkLoopP1RootYStartBits = 0;
    gNdsFighterWalkLoopP1RootYAfterHeldBits = 0;
    gNdsFighterWalkLoopP1RootYAfterSettleBits = 0;
    gNdsFighterWalkLoopP0RootDeltaXMilli = 0;
    gNdsFighterWalkLoopP1RootDeltaXMilli = 0;
    gNdsFighterWalkLoopP0HeldRootDeltaXMilli = 0;
    gNdsFighterWalkLoopP1HeldRootDeltaXMilli = 0;
    gNdsFighterWalkLoopP0RootDirectionOK = 0;
    gNdsFighterWalkLoopP1RootDirectionOK = 0;
    gNdsFighterWalkLoopP0GroundVelStartMilli = 0;
    gNdsFighterWalkLoopP1GroundVelStartMilli = 0;
    gNdsFighterWalkLoopP0GroundVelAfterHeldMilli = 0;
    gNdsFighterWalkLoopP1GroundVelAfterHeldMilli = 0;
    gNdsFighterWalkLoopP0GroundVelAfterSettleMilli = 0;
    gNdsFighterWalkLoopP1GroundVelAfterSettleMilli = 0;
    gNdsFighterWalkLoopP0AirVelXAfterHeldMilli = 0;
    gNdsFighterWalkLoopP1AirVelXAfterHeldMilli = 0;
    gNdsFighterWalkLoopP0AirVelYAfterHeldMilli = 0;
    gNdsFighterWalkLoopP1AirVelYAfterHeldMilli = 0;
    gNdsFighterWalkLoopGroundVelAbsStickCount = 0;
    gNdsFighterWalkLoopGroundVelTransferAirCount = 0;
    gNdsFighterWalkLoopWaitReturnCheckCount = 0;
    gNdsFighterWalkLoopWaitReturnSuccessCount = 0;
    gNdsFighterWalkLoopWaitSetStatusCount = 0;
    gNdsFighterWalkLoopWaitFrictionCount = 0;
    gNdsFighterWalkLoopReleaseInputCount = 0;
    gNdsFighterWalkLoopMapSafeFloorCount = 0;
    gNdsFighterWalkLoopMapFallDeniedCount = 0;
    gNdsFighterWalkLoopMapOttottoDeniedCount = 0;
    gNdsFighterWalkLoopGObjDelta = 0;
    gNdsFighterWalkLoopUnexpectedStatusCount = 0;
    gNdsFighterWalkLoopDeniedStatusCount = 0;
    gNdsFighterWalkLoopProcessAttachCount = 0;
    gNdsFighterWalkLoopDisplayProbeCount = 0;
    gNdsFighterWalkLoopGameplayUpdateCount = 0;
    gNdsFighterWalkLoopDrawCallCount = 0;
    gNdsFighterWalkLoopMatrixCallCount = 0;
    gNdsFighterWalkLoopRootYDriftCount = 0;
    gNdsFighterWalkLoopGADriftCount = 0;
    gNdsFighterMarioFoxDashRunResult = 0;
    gNdsFighterMarioFoxDashRunSafeResult = 0;
    gNdsFighterMarioFoxDashRunMask = 0;
    gNdsFighterMarioFoxDashRunDeferredMask = 0;
    gNdsFighterMarioFoxDashRunCount = 0;
    gNdsFighterDashRunWaitInterruptCallCount = 0;
    gNdsFighterDashRunGroundCheckCallCount = 0;
    gNdsFighterDashRunOriginalDashCheckCallCount = 0;
    gNdsFighterDashRunOriginalDashCheckSuccessCount = 0;
    gNdsFighterDashRunAttack1CheckCallCount = 0;
    gNdsFighterDashRunAttack1CheckSuccessCount = 0;
    gNdsFighterDashRunAttack100StartCheckCallCount = 0;
    gNdsFighterDashRunAttackDashCheckCallCount = 0;
    gNdsFighterDashRunAttackDashCheckSuccessCount = 0;
    gNdsFighterDashRunDashSetStatusCount = 0;
    gNdsFighterDashRunRunSetStatusCount = 0;
    gNdsFighterDashRunRunBrakeSetStatusCount = 0;
    gNdsFighterDashRunAttack11SetStatusCount = 0;
    gNdsFighterDashRunAttack12SetStatusCount = 0;
    gNdsFighterDashRunAttack13SetStatusCount = 0;
    gNdsFighterDashRunAttack100StartSetStatusCount = 0;
    gNdsFighterDashRunAttack100LoopSetStatusCount = 0;
    gNdsFighterDashRunAttackDashSetStatusCount = 0;
    gNdsFighterDashRunDashInterruptCount = 0;
    gNdsFighterDashRunRunInterruptCount = 0;
    gNdsFighterDashRunRunBrakeInterruptCount = 0;
    gNdsFighterDashRunDashPhysicsCount = 0;
    gNdsFighterDashRunRunPhysicsCount = 0;
    gNdsFighterDashRunRunBrakePhysicsCount = 0;
    gNdsFighterDashRunDashMapCount = 0;
    gNdsFighterDashRunRunMapCount = 0;
    gNdsFighterDashRunRunBrakeMapCount = 0;
    gNdsFighterDashRunSafeFloorCount = 0;
    gNdsFighterDashRunFallBreakSafeCount = 0;
    gNdsFighterDashRunDeferredInterruptCount = 0;
    gNdsFighterDashRunFtMainDashStatusCount = 0;
    gNdsFighterDashRunFtMainRunStatusCount = 0;
    gNdsFighterDashRunFtMainRunBrakeStatusCount = 0;
    gNdsFighterDashRunFtMainAttack11StatusCount = 0;
    gNdsFighterDashRunFtMainAttack12StatusCount = 0;
    gNdsFighterDashRunFtMainAttack13StatusCount = 0;
    gNdsFighterDashRunFtMainAttack100StartStatusCount = 0;
    gNdsFighterDashRunFtMainAttack100LoopStatusCount = 0;
    gNdsFighterDashRunFtMainAttackDashStatusCount = 0;
    gNdsFighterDashRunAnimEventsCallCount = 0;
    gNdsFighterDashRunGroundVelFrictionCount = 0;
    gNdsFighterDashRunGroundVelTransferAirCount = 0;
    gNdsFighterDashRunP0StatusDash = 0xffffffffu;
    gNdsFighterDashRunP1StatusDash = 0xffffffffu;
    gNdsFighterDashRunP0MotionDash = 0xffffffffu;
    gNdsFighterDashRunP1MotionDash = 0xffffffffu;
    gNdsFighterDashRunP0StatusRun = 0xffffffffu;
    gNdsFighterDashRunP1StatusRun = 0xffffffffu;
    gNdsFighterDashRunP0MotionRun = 0xffffffffu;
    gNdsFighterDashRunP1MotionRun = 0xffffffffu;
    gNdsFighterDashRunP0StatusRunBrake = 0xffffffffu;
    gNdsFighterDashRunP1StatusRunBrake = 0xffffffffu;
    gNdsFighterDashRunP0MotionRunBrake = 0xffffffffu;
    gNdsFighterDashRunP1MotionRunBrake = 0xffffffffu;
    gNdsFighterDashRunP0StatusAttack11 = 0xffffffffu;
    gNdsFighterDashRunP1StatusAttack11 = 0xffffffffu;
    gNdsFighterDashRunP0MotionAttack11 = 0xffffffffu;
    gNdsFighterDashRunP1MotionAttack11 = 0xffffffffu;
    gNdsFighterDashRunP0StatusAttack12 = 0xffffffffu;
    gNdsFighterDashRunP1StatusAttack12 = 0xffffffffu;
    gNdsFighterDashRunP0MotionAttack12 = 0xffffffffu;
    gNdsFighterDashRunP1MotionAttack12 = 0xffffffffu;
    gNdsFighterDashRunP0StatusAttack13 = 0xffffffffu;
    gNdsFighterDashRunP1StatusAttack13 = 0xffffffffu;
    gNdsFighterDashRunP0MotionAttack13 = 0xffffffffu;
    gNdsFighterDashRunP1MotionAttack13 = 0xffffffffu;
    gNdsFighterDashRunP0StatusAttack100Start = 0xffffffffu;
    gNdsFighterDashRunP1StatusAttack100Start = 0xffffffffu;
    gNdsFighterDashRunP0MotionAttack100Start = 0xffffffffu;
    gNdsFighterDashRunP1MotionAttack100Start = 0xffffffffu;
    gNdsFighterDashRunP1StatusAttack100Loop = 0xffffffffu;
    gNdsFighterDashRunP1MotionAttack100Loop = 0xffffffffu;
    gNdsFighterDashRunP0StatusAttackDash = 0xffffffffu;
    gNdsFighterDashRunP1StatusAttackDash = 0xffffffffu;
    gNdsFighterDashRunP0MotionAttackDash = 0xffffffffu;
    gNdsFighterDashRunP1MotionAttackDash = 0xffffffffu;
    gNdsFighterDashRunAttack11CallbackMask = 0;
    gNdsFighterDashRunAttack11TickMask = 0;
    gNdsFighterDashRunAttack11WaitProcMask = 0;
    gNdsFighterDashRunAttack12CallbackMask = 0;
    gNdsFighterDashRunAttack12GotoMask = 0;
    gNdsFighterDashRunAttack13CallbackMask = 0;
    gNdsFighterDashRunAttack13GotoMask = 0;
    gNdsFighterDashRunAttack100StartCallbackMask = 0;
    gNdsFighterDashRunAttack100StartGotoMask = 0;
    gNdsFighterDashRunAttack100LoopCallbackMask = 0;
    gNdsFighterDashRunAttack100LoopGotoMask = 0;
    gNdsFighterDashRunAttack100LoopTickMask = 0;
    gNdsFighterDashRunAttackAnimEventsMask = 0;
    gNdsFighterDashRunAttackEventMask = 0;
    gNdsFighterDashRunAttackEventScriptMask = 0;
    gNdsFighterDashRunAttackEventNoHitMask = 0;
    gNdsFighterDashRunAttackEventCommandMask = 0;
    gNdsFighterDashRunAttackEventParseCount = 0;
    gNdsFighterDashRunAttackEventLastPlayer = 0xffffffffu;
    gNdsFighterDashRunAttackEventLastStatus = 0xffffffffu;
    gNdsFighterDashRunAttackEventLastState = 0xffffffffu;
    gNdsFighterDashRunAttackEventLastAttackID = 0xffffffffu;
    gNdsFighterDashRunAttackEventLastGroupID = 0xffffffffu;
    gNdsFighterDashRunAttackEventLastJointID = 0xffffffffu;
    gNdsFighterDashRunAttackEventLastDamage = 0;
    gNdsFighterDashRunAttackEventLastSize = 0;
    gNdsFighterDashRunAttackEventLastOffsetX = 0;
    gNdsFighterDashRunAttackEventLastOffsetY = 0;
    gNdsFighterDashRunAttackEventLastOffsetZ = 0;
    gNdsFighterDashRunAttackEventLastAngle = 0;
    gNdsFighterDashRunAttackEventLastKBG = 0;
    gNdsFighterDashRunAttackEventLastKBW = 0;
    gNdsFighterDashRunAttackEventLastBKB = 0;
    gNdsFighterDashRunAttackEventLastShield = 0;
    gNdsFighterDashRunAttackEventLastFlags = 0;
    gNdsFighterDashRunAttackEventPositionMask = 0;
    gNdsFighterDashRunAttackEventPositionState = 0xffffffffu;
    gNdsFighterDashRunAttackEventPositionAttackID = 0xffffffffu;
    gNdsFighterDashRunAttackEventPositionJointID = 0xffffffffu;
    gNdsFighterDashRunAttackEventPositionX = 0;
    gNdsFighterDashRunAttackEventPositionY = 0;
    gNdsFighterDashRunAttackEventPositionZ = 0;
    gNdsFighterDashRunAttackEventPositionMatrixFlag = 0;
    gNdsFighterDashRunAttackEventPositionMatrixValue = 0;
    gNdsFighterDashRunDamageStatusMask = 0;
    gNdsFighterDashRunDamageStatusLevel = 0xffffffffu;
    gNdsFighterDashRunDamageStatusIndex = 0xffffffffu;
    gNdsFighterDashRunDamageStatusGround = 0xffffffffu;
    gNdsFighterDashRunDamageStatusAir = 0xffffffffu;
    gNdsFighterDashRunDamageStatusElectric = 0xffffffffu;
    gNdsFighterDashRunDamageSetupMask = 0;
    gNdsFighterDashRunDamageSetupStatusBefore = 0xffffffffu;
    gNdsFighterDashRunDamageSetupStatusAfter = 0xffffffffu;
    gNdsFighterDashRunDamageSetupMotionAfter = 0xffffffffu;
    gNdsFighterDashRunDamageSetupGAAfter = 0xffffffffu;
    gNdsFighterDashRunDamageSetupHitstunBefore = -1;
    gNdsFighterDashRunDamageSetupHitstunAfter = -1;
    gNdsFighterDashRunDamageSetupVelGroundMilli = 0;
    gNdsFighterDashRunDamageSetupVelAirXMilli = 0;
    gNdsFighterDashRunDamageSetupVelAirYMilli = 0;
    gNdsFighterDashRunDamageSetupVelPhysicsMilli = 0;
    gNdsFighterDashRunGuardCheckCallCount = 0;
    gNdsFighterDashRunGuardCheckSuccessCount = 0;
    gNdsFighterDashRunGuardSetStatusCount = 0;
    gNdsFighterDashRunFtMainGuardOnStatusCount = 0;
    gNdsFighterDashRunGuardSetOffSetStatusCount = 0;
    gNdsFighterDashRunFtMainGuardSetOffStatusCount = 0;
    gNdsFighterDashRunGuardAnimEventsMask = 0;
    gNdsFighterDashRunGuardEffectCount = 0;
    gNdsFighterDashRunGuardFGMCount = 0;
    gNdsFighterDashRunGuardLastFGM = 0;
    gNdsFighterDashRunP0StatusGuardOn = 0;
    gNdsFighterDashRunP1StatusGuardOn = 0;
    gNdsFighterDashRunP0MotionGuardOn = 0;
    gNdsFighterDashRunP1MotionGuardOn = 0;
    gNdsFighterDashRunGuardCallbackMask = 0;
    gNdsFighterDashRunGuardStateMask = 0;
    gNdsFighterDashRunGuardSetOffMask = 0;
    gNdsFighterDashRunGuardSetOffCallbackMask = 0;
    gNdsFighterDashRunGuardSetOffFramesMilli = 0;
    gNdsFighterDashRunGuardSetOffVelMilli = 0;
    gNdsFighterDashRunEscapeCheckCallCount = 0;
    gNdsFighterDashRunEscapeCheckSuccessCount = 0;
    gNdsFighterDashRunEscapeSetStatusCount = 0;
    gNdsFighterDashRunFtMainEscapeStatusCount = 0;
    gNdsFighterDashRunEscapeCallbackMask = 0;
    gNdsFighterDashRunEscapeStateMask = 0;
    gNdsFighterDashRunEscapeTickMask = 0;
    gNdsFighterDashRunEscapeInterruptCount = 0;
    gNdsFighterDashRunEscapePhysicsCount = 0;
    gNdsFighterDashRunEscapeMapCount = 0;
    gNdsFighterDashRunP0StatusEscape = 0;
    gNdsFighterDashRunP1StatusEscape = 0;
    gNdsFighterDashRunP0MotionEscape = 0;
    gNdsFighterDashRunP1MotionEscape = 0;
    gNdsFighterDashRunP0EscapeItemThrowBuffer = 0;
    gNdsFighterDashRunP1EscapeItemThrowBuffer = 0;
    gNdsFighterDashRunAttackDashCallbackMask = 0;
    gNdsFighterDashRunAttackDashTickMask = 0;
    gNdsFighterDashRunAttackDashRunProcMask = 0;
    gNdsFighterDashRunP0TapStickXAfterDash = 0;
    gNdsFighterDashRunP1TapStickXAfterDash = 0;
    gNdsFighterDashRunP0LR = 0;
    gNdsFighterDashRunP1LR = 0;
    gNdsFighterDashRunP0StickX = 0;
    gNdsFighterDashRunP1StickX = 0;
    gNdsFighterDashRunP0RootDeltaXMilli = 0;
    gNdsFighterDashRunP1RootDeltaXMilli = 0;
    gNdsFighterDashRunP0GroundVelRunMilli = 0;
    gNdsFighterDashRunP1GroundVelRunMilli = 0;
    gNdsFighterDashRunP0GroundVelBrakeMilli = 0;
    gNdsFighterDashRunP1GroundVelBrakeMilli = 0;
    gNdsFighterDashRunP0RootDirectionOK = 0;
    gNdsFighterDashRunP1RootDirectionOK = 0;
    gNdsFighterDashRunRootYDriftCount = 0;
    gNdsFighterDashRunGADriftCount = 0;
    gNdsFighterDashRunGObjDelta = 0;
    gNdsFighterDashRunDeniedStatusCount = 0;
    gNdsFighterDashRunUnexpectedStatusCount = 0;
    gNdsFighterDashRunProcessAttachCount = 0;
    gNdsFighterDashRunDisplayProbeCount = 0;
    gNdsFighterDashRunGameplayUpdateCount = 0;
    gNdsFighterDashRunDrawCallCount = 0;
    gNdsFighterDashRunMatrixCallCount = 0;
    gNdsFighterMarioFoxJumpLoopResult = 0;
    gNdsFighterMarioFoxJumpLoopSafeResult = 0;
    gNdsFighterMarioFoxJumpLoopMask = 0;
    gNdsFighterMarioFoxJumpLoopDeferredMask = 0;
    gNdsFighterMarioFoxJumpLoopCount = 0;
    gNdsFighterJumpRunBrakeEndCallCount = 0;
    gNdsFighterJumpWaitSetStatusCount = 0;
    gNdsFighterJumpWaitInterruptCallCount = 0;
    gNdsFighterJumpGroundCheckCallCount = 0;
    gNdsFighterJumpOriginalKneeBendCheckCallCount = 0;
    gNdsFighterJumpOriginalKneeBendCheckSuccessCount = 0;
    gNdsFighterJumpKneeBendSetStatusCallCount = 0;
    gNdsFighterJumpFtMainKneeBendStatusCount = 0;
    gNdsFighterJumpKneeBendUpdateCallCount = 0;
    gNdsFighterJumpKneeBendInterruptCallCount = 0;
    gNdsFighterJumpSetStatusCallCount = 0;
    gNdsFighterJumpFtMainJumpStatusCount = 0;
    gNdsFighterJumpSetAirCallCount = 0;
    gNdsFighterJumpAirInterruptCallCount = 0;
    gNdsFighterJumpAirPhysicsCallCount = 0;
    gNdsFighterJumpAirMapCallCount = 0;
    gNdsFighterJumpGravityCallCount = 0;
    gNdsFighterJumpAirDriftCallCount = 0;
    gNdsFighterJumpAirFrictionCallCount = 0;
    gNdsFighterJumpDeferredInterruptCheckCount = 0;
    gNdsFighterJumpSpecialHiCheckCount = 0;
    gNdsFighterJumpAttackHi4KneeBendCheckCount = 0;
    gNdsFighterJumpSpecialAirCheckCount = 0;
    gNdsFighterJumpAttackAirCheckCount = 0;
    gNdsFighterJumpAttackAirRefreshCount = 0;
    gNdsFighterJumpAttackAirRefreshMask = 0;
    gNdsFighterJumpAttackAirRefreshStateMask = 0;
    gNdsFighterJumpAttackAirRecordClearMask = 0;
    gNdsFighterJumpAttackAirMapLandingMask = 0;
    gNdsFighterJumpAttackAirDirectionMask = 0;
    gNdsFighterJumpAerialCheckCount = 0;
    gNdsFighterJumpHammerHoldCheckCount = 0;
    gNdsFighterJumpHammerKneeBendCheckCount = 0;
    gNdsFighterJumpFallDeferredCount = 0;
    gNdsFighterJumpLandingDeniedCount = 0;
    gNdsFighterJumpCliffDeniedCount = 0;
    gNdsFighterJumpCeilingDeniedCount = 0;
    gNdsFighterJumpDeniedStatusCount = 0;
    gNdsFighterJumpUnexpectedStatusCount = 0;
    gNdsFighterJumpProcessAttachCount = 0;
    gNdsFighterJumpDisplayProbeCount = 0;
    gNdsFighterJumpGameplayUpdateCount = 0;
    gNdsFighterJumpDrawCallCount = 0;
    gNdsFighterJumpMatrixCallCount = 0;
    gNdsFighterJumpP0StatusStart = 0xffffffffu;
    gNdsFighterJumpP1StatusStart = 0xffffffffu;
    gNdsFighterJumpP0MotionStart = 0xffffffffu;
    gNdsFighterJumpP1MotionStart = 0xffffffffu;
    gNdsFighterJumpP0StatusWait = 0xffffffffu;
    gNdsFighterJumpP1StatusWait = 0xffffffffu;
    gNdsFighterJumpP0MotionWait = 0xffffffffu;
    gNdsFighterJumpP1MotionWait = 0xffffffffu;
    gNdsFighterJumpP0StatusKneeBend = 0xffffffffu;
    gNdsFighterJumpP1StatusKneeBend = 0xffffffffu;
    gNdsFighterJumpP0MotionKneeBend = 0xffffffffu;
    gNdsFighterJumpP1MotionKneeBend = 0xffffffffu;
    gNdsFighterJumpP0StatusJump = 0xffffffffu;
    gNdsFighterJumpP1StatusJump = 0xffffffffu;
    gNdsFighterJumpP0MotionJump = 0xffffffffu;
    gNdsFighterJumpP1MotionJump = 0xffffffffu;
    gNdsFighterJumpP0GAStart = 0xffffffffu;
    gNdsFighterJumpP1GAStart = 0xffffffffu;
    gNdsFighterJumpP0GAWait = 0xffffffffu;
    gNdsFighterJumpP1GAWait = 0xffffffffu;
    gNdsFighterJumpP0GAKneeBend = 0xffffffffu;
    gNdsFighterJumpP1GAKneeBend = 0xffffffffu;
    gNdsFighterJumpP0GAJump = 0xffffffffu;
    gNdsFighterJumpP1GAJump = 0xffffffffu;
    gNdsFighterJumpP0GAAfterAir = 0xffffffffu;
    gNdsFighterJumpP1GAAfterAir = 0xffffffffu;
    gNdsFighterJumpP0InputSource = 0;
    gNdsFighterJumpP1InputSource = 0;
    gNdsFighterJumpP0ShortHop = 0;
    gNdsFighterJumpP1ShortHop = 0;
    gNdsFighterJumpP0StickX = 0;
    gNdsFighterJumpP1StickX = 0;
    gNdsFighterJumpP0ButtonTap = 0;
    gNdsFighterJumpP1ButtonTap = 0;
    gNdsFighterJumpP0ButtonRelease = 0;
    gNdsFighterJumpP1ButtonRelease = 0;
    gNdsFighterJumpP0KneeBendFrames = 0;
    gNdsFighterJumpP1KneeBendFrames = 0;
    gNdsFighterJumpP0AirFrames = 0;
    gNdsFighterJumpP1AirFrames = 0;
    gNdsFighterJumpP0RootDeltaXMilli = 0;
    gNdsFighterJumpP1RootDeltaXMilli = 0;
    gNdsFighterJumpP0RootDeltaYMilli = 0;
    gNdsFighterJumpP1RootDeltaYMilli = 0;
    gNdsFighterJumpP0RootDirectionOK = 0;
    gNdsFighterJumpP1RootDirectionOK = 0;
    gNdsFighterJumpP0RootRiseOK = 0;
    gNdsFighterJumpP1RootRiseOK = 0;
    gNdsFighterJumpP0VelXInitialMilli = 0;
    gNdsFighterJumpP1VelXInitialMilli = 0;
    gNdsFighterJumpP0VelYInitialMilli = 0;
    gNdsFighterJumpP1VelYInitialMilli = 0;
    gNdsFighterJumpP0VelXAfterMilli = 0;
    gNdsFighterJumpP1VelXAfterMilli = 0;
    gNdsFighterJumpP0VelYAfterMilli = 0;
    gNdsFighterJumpP1VelYAfterMilli = 0;
    gNdsFighterJumpGObjDelta = 0;
    gNdsFighterMarioFoxLandingLoopResult = 0;
    gNdsFighterMarioFoxLandingLoopSafeResult = 0;
    gNdsFighterMarioFoxLandingLoopMask = 0;
    gNdsFighterMarioFoxLandingLoopDeferredMask = 0;
    gNdsFighterMarioFoxLandingLoopCount = 0;
    gNdsFighterLandingJumpAnimEndCallCount = 0;
    gNdsFighterLandingFallSetStatusCallCount = 0;
    gNdsFighterLandingFtMainFallStatusCount = 0;
    gNdsFighterLandingSetGroundCallCount = 0;
    gNdsFighterLandingSetStatusCallCount = 0;
    gNdsFighterLandingFtMainLandingLightStatusCount = 0;
    gNdsFighterLandingFtMainLandingHeavyStatusCount = 0;
    gNdsFighterLandingEndCallCount = 0;
    gNdsFighterLandingWaitSetStatusCount = 0;
    gNdsFighterLandingWaitSetStatusSuccessCount = 0;
    gNdsFighterLandingFallFrameMax = 0;
    gNdsFighterLandingLandingFrameTarget = 0;
    gNdsFighterLandingP0FallFrameCount = 0;
    gNdsFighterLandingP1FallFrameCount = 0;
    gNdsFighterLandingP0FallInterruptCount = 0;
    gNdsFighterLandingP1FallInterruptCount = 0;
    gNdsFighterLandingP0FallPhysicsCount = 0;
    gNdsFighterLandingP1FallPhysicsCount = 0;
    gNdsFighterLandingP0FallMapCount = 0;
    gNdsFighterLandingP1FallMapCount = 0;
    gNdsFighterLandingP0LandingFrameCount = 0;
    gNdsFighterLandingP1LandingFrameCount = 0;
    gNdsFighterLandingP0LandingInterruptCount = 0;
    gNdsFighterLandingP1LandingInterruptCount = 0;
    gNdsFighterLandingP0LandingPhysicsCount = 0;
    gNdsFighterLandingP1LandingPhysicsCount = 0;
    gNdsFighterLandingAirNoCollisionCount = 0;
    gNdsFighterLandingFloorDetectCount = 0;
    gNdsFighterLandingFloorClampCount = 0;
    gNdsFighterLandingFastFallCheckCount = 0;
    gNdsFighterLandingFastFallCount = 0;
    gNdsFighterLandingHeavyDeniedCount = 0;
    gNdsFighterLandingFallAerialDeniedCount = 0;
    gNdsFighterLandingJumpAerialDeniedCount = 0;
    gNdsFighterLandingCliffDeniedCount = 0;
    gNdsFighterLandingCeilingDeniedCount = 0;
    gNdsFighterLandingDeferredInterruptCheckCount = 0;
    gNdsFighterLandingGObjDelta = 0;
    gNdsFighterLandingUnexpectedStatusCount = 0;
    gNdsFighterLandingDeniedStatusCount = 0;
    gNdsFighterLandingProcessAttachCount = 0;
    gNdsFighterLandingDisplayProbeCount = 0;
    gNdsFighterLandingGameplayUpdateCount = 0;
    gNdsFighterLandingDrawCallCount = 0;
    gNdsFighterLandingMatrixCallCount = 0;
    gNdsFighterLandingRootYDriftCount = 0;
    gNdsFighterLandingGADriftCount = 0;
    gNdsFighterLandingP0StatusStart = 0xffffffffu;
    gNdsFighterLandingP1StatusStart = 0xffffffffu;
    gNdsFighterLandingP0MotionStart = 0xffffffffu;
    gNdsFighterLandingP1MotionStart = 0xffffffffu;
    gNdsFighterLandingP0GAStart = 0xffffffffu;
    gNdsFighterLandingP1GAStart = 0xffffffffu;
    gNdsFighterLandingP0StatusFall = 0xffffffffu;
    gNdsFighterLandingP1StatusFall = 0xffffffffu;
    gNdsFighterLandingP0MotionFall = 0xffffffffu;
    gNdsFighterLandingP1MotionFall = 0xffffffffu;
    gNdsFighterLandingP0GAFall = 0xffffffffu;
    gNdsFighterLandingP1GAFall = 0xffffffffu;
    gNdsFighterLandingP0StatusLanding = 0xffffffffu;
    gNdsFighterLandingP1StatusLanding = 0xffffffffu;
    gNdsFighterLandingP0MotionLanding = 0xffffffffu;
    gNdsFighterLandingP1MotionLanding = 0xffffffffu;
    gNdsFighterLandingP0GALanding = 0xffffffffu;
    gNdsFighterLandingP1GALanding = 0xffffffffu;
    gNdsFighterLandingP0StatusWait = 0xffffffffu;
    gNdsFighterLandingP1StatusWait = 0xffffffffu;
    gNdsFighterLandingP0MotionWait = 0xffffffffu;
    gNdsFighterLandingP1MotionWait = 0xffffffffu;
    gNdsFighterLandingP0GAWait = 0xffffffffu;
    gNdsFighterLandingP1GAWait = 0xffffffffu;
    gNdsFighterLandingP0FloorYMilli = 0;
    gNdsFighterLandingP1FloorYMilli = 0;
    gNdsFighterLandingP0RootYFallStartMilli = 0;
    gNdsFighterLandingP1RootYFallStartMilli = 0;
    gNdsFighterLandingP0RootYFinalMilli = 0;
    gNdsFighterLandingP1RootYFinalMilli = 0;
    gNdsFighterLandingP0RootDeltaXMilli = 0;
    gNdsFighterLandingP1RootDeltaXMilli = 0;
    gNdsFighterLandingP0RootDirectionOK = 0;
    gNdsFighterLandingP1RootDirectionOK = 0;
    gNdsFighterLandingP0RootFloorOK = 0;
    gNdsFighterLandingP1RootFloorOK = 0;
    gNdsFighterLandingP0VelYFallStartMilli = 0;
    gNdsFighterLandingP1VelYFallStartMilli = 0;
    gNdsFighterLandingP0VelYBeforeLandingMilli = 0;
    gNdsFighterLandingP1VelYBeforeLandingMilli = 0;
    gNdsFighterLandingP0GroundVelAfterLandingMilli = 0;
    gNdsFighterLandingP1GroundVelAfterLandingMilli = 0;
    gNdsFighterLandingP0GroundVelAfterWaitMilli = 0;
    gNdsFighterLandingP1GroundVelAfterWaitMilli = 0;
    gNdsFighterLandingGravityCallCount = 0;
    gNdsFighterLandingAirDriftCallCount = 0;
    gNdsFighterLandingAirFrictionCallCount = 0;
    gNdsFighterLandingGroundFrictionCallCount = 0;
    gNdsFighterLandingWaitFrictionCallCount = 0;
    gNdsFighterMarioFoxProcessLoopResult = 0;
    gNdsFighterMarioFoxProcessLoopSafeResult = 0;
    gNdsFighterMarioFoxProcessLoopMask = 0;
    gNdsFighterMarioFoxProcessLoopDeferredMask = 0;
    gNdsFighterMarioFoxProcessLoopCount = 0;
    gNdsFighterProcessLoopFrameMax = 0;
    gNdsFighterProcessLoopP0FrameCount = 0;
    gNdsFighterProcessLoopP1FrameCount = 0;
    gNdsFighterProcessLoopP0Completed = 0;
    gNdsFighterProcessLoopP1Completed = 0;
    gNdsFighterProcessLoopP0StatusVisitMask = 0;
    gNdsFighterProcessLoopP1StatusVisitMask = 0;
    gNdsFighterProcessLoopP0TransitionMask = 0;
    gNdsFighterProcessLoopP1TransitionMask = 0;
    gNdsFighterProcessLoopP0InputApplyCount = 0;
    gNdsFighterProcessLoopP1InputApplyCount = 0;
    gNdsFighterProcessLoopControllerBridgeCount = 0;
    gNdsFighterProcessLoopControllerMirrorCount = 0;
    gNdsFighterProcessLoopP0ButtonTapMask = 0;
    gNdsFighterProcessLoopP1ButtonTapMask = 0;
    gNdsFighterProcessLoopP0LastStickX = 0;
    gNdsFighterProcessLoopP1LastStickX = 0;
    gNdsFighterProcessLoopP0UpdateCount = 0;
    gNdsFighterProcessLoopP1UpdateCount = 0;
    gNdsFighterProcessLoopP0InterruptCount = 0;
    gNdsFighterProcessLoopP1InterruptCount = 0;
    gNdsFighterProcessLoopP0PhysicsCount = 0;
    gNdsFighterProcessLoopP1PhysicsCount = 0;
    gNdsFighterProcessLoopP0IntegrateCount = 0;
    gNdsFighterProcessLoopP1IntegrateCount = 0;
    gNdsFighterProcessLoopP0MapCount = 0;
    gNdsFighterProcessLoopP1MapCount = 0;
    gNdsFighterProcessLoopP0WaitVisitCount = 0;
    gNdsFighterProcessLoopP1WaitVisitCount = 0;
    gNdsFighterProcessLoopP0WalkVisitCount = 0;
    gNdsFighterProcessLoopP1WalkVisitCount = 0;
    gNdsFighterProcessLoopP0DashVisitCount = 0;
    gNdsFighterProcessLoopP1DashVisitCount = 0;
    gNdsFighterProcessLoopP0RunVisitCount = 0;
    gNdsFighterProcessLoopP1RunVisitCount = 0;
    gNdsFighterProcessLoopP0RunBrakeVisitCount = 0;
    gNdsFighterProcessLoopP1RunBrakeVisitCount = 0;
    gNdsFighterProcessLoopP0KneeBendVisitCount = 0;
    gNdsFighterProcessLoopP1KneeBendVisitCount = 0;
    gNdsFighterProcessLoopP0JumpVisitCount = 0;
    gNdsFighterProcessLoopP1JumpVisitCount = 0;
    gNdsFighterProcessLoopP0FallVisitCount = 0;
    gNdsFighterProcessLoopP1FallVisitCount = 0;
    gNdsFighterProcessLoopP0LandingVisitCount = 0;
    gNdsFighterProcessLoopP1LandingVisitCount = 0;
    gNdsFighterProcessLoopP0StatusStart = 0xffffffffu;
    gNdsFighterProcessLoopP1StatusStart = 0xffffffffu;
    gNdsFighterProcessLoopP0MotionStart = 0xffffffffu;
    gNdsFighterProcessLoopP1MotionStart = 0xffffffffu;
    gNdsFighterProcessLoopP0StatusFinal = 0xffffffffu;
    gNdsFighterProcessLoopP1StatusFinal = 0xffffffffu;
    gNdsFighterProcessLoopP0MotionFinal = 0xffffffffu;
    gNdsFighterProcessLoopP1MotionFinal = 0xffffffffu;
    gNdsFighterProcessLoopP0GAFinal = 0xffffffffu;
    gNdsFighterProcessLoopP1GAFinal = 0xffffffffu;
    gNdsFighterProcessLoopP0FloorYMilli = 0;
    gNdsFighterProcessLoopP1FloorYMilli = 0;
    gNdsFighterProcessLoopP0RootXStartMilli = 0;
    gNdsFighterProcessLoopP1RootXStartMilli = 0;
    gNdsFighterProcessLoopP0RootXFinalMilli = 0;
    gNdsFighterProcessLoopP1RootXFinalMilli = 0;
    gNdsFighterProcessLoopP0RootDeltaXMilli = 0;
    gNdsFighterProcessLoopP1RootDeltaXMilli = 0;
    gNdsFighterProcessLoopP0RootYFinalMilli = 0;
    gNdsFighterProcessLoopP1RootYFinalMilli = 0;
    gNdsFighterProcessLoopP0RootRiseMilli = 0;
    gNdsFighterProcessLoopP1RootRiseMilli = 0;
    gNdsFighterProcessLoopP0RootDirectionOK = 0;
    gNdsFighterProcessLoopP1RootDirectionOK = 0;
    gNdsFighterProcessLoopP0FloorOK = 0;
    gNdsFighterProcessLoopP1FloorOK = 0;
    gNdsFighterProcessLoopP0GroundVelFinalMilli = 0;
    gNdsFighterProcessLoopP1GroundVelFinalMilli = 0;
    gNdsFighterProcessLoopP0AirVelXFinalMilli = 0;
    gNdsFighterProcessLoopP1AirVelXFinalMilli = 0;
    gNdsFighterProcessLoopP0AirVelYFinalMilli = 0;
    gNdsFighterProcessLoopP1AirVelYFinalMilli = 0;
    gNdsFighterProcessLoopFallDetectCount = 0;
    gNdsFighterProcessLoopLandingDetectCount = 0;
    gNdsFighterProcessLoopSetGroundCount = 0;
    gNdsFighterProcessLoopSetAirCount = 0;
    gNdsFighterProcessLoopWaitSetStatusCount = 0;
    gNdsFighterProcessLoopRunBrakeEndCount = 0;
    gNdsFighterProcessLoopJumpAnimEndCount = 0;
    gNdsFighterProcessLoopLandingEndCount = 0;
    gNdsFighterProcessLoopDeferredInterruptCheckCount = 0;
    gNdsFighterProcessLoopGObjDelta = 0;
    gNdsFighterProcessLoopUnexpectedStatusCount = 0;
    gNdsFighterProcessLoopDeniedStatusCount = 0;
    gNdsFighterProcessLoopProcessAttachCount = 0;
    gNdsFighterProcessLoopDisplayProbeCount = 0;
    gNdsFighterProcessLoopGameplayUpdateCount = 0;
    gNdsFighterProcessLoopDrawCallCount = 0;
    gNdsFighterProcessLoopMatrixCallCount = 0;
    gNdsFighterProcessLoopRootYDriftCount = 0;
    gNdsFighterProcessLoopGADriftCount = 0;
    gNdsFighterMarioFoxSchedulerLoopResult = 0;
    gNdsFighterMarioFoxSchedulerLoopSafeResult = 0;
    gNdsFighterMarioFoxSchedulerLoopMask = 0;
    gNdsFighterMarioFoxSchedulerLoopDeferredMask = 0;
    gNdsFighterMarioFoxSchedulerLoopCount = 0;
    gNdsFighterSchedulerLoopPrepared = 0;
    gNdsFighterSchedulerLoopFrameMax = 0;
    gNdsFighterSchedulerLoopUpdateMax = 0;
    gNdsFighterSchedulerLoopTaskmanUpdateCount = 0;
    gNdsFighterSchedulerLoopVSBattleUpdateCount = 0;
    gNdsFighterSchedulerLoopBaseVSBattleUpdateCount = 0;
    gNdsFighterSchedulerLoopSchedulerUpdateCount = 0;
    gNdsFighterSchedulerLoopGObjCountBefore = 0;
    gNdsFighterSchedulerLoopGObjCountAfter = 0;
    gNdsFighterSchedulerLoopGObjDelta = 0;
    gNdsFighterSchedulerLoopP0ProcessAttachCount = 0;
    gNdsFighterSchedulerLoopP1ProcessAttachCount = 0;
    gNdsFighterSchedulerLoopProcessAttachEscapeCount = 0;
    gNdsFighterSchedulerLoopP0GObjProcessRunCount = 0;
    gNdsFighterSchedulerLoopP1GObjProcessRunCount = 0;
    gNdsFighterSchedulerLoopP0ProcCallbackCount = 0;
    gNdsFighterSchedulerLoopP1ProcCallbackCount = 0;
    gNdsFighterSchedulerLoopP0InputApplyCount = 0;
    gNdsFighterSchedulerLoopP1InputApplyCount = 0;
    gNdsFighterSchedulerLoopControllerBridgeCount = 0;
    gNdsFighterSchedulerLoopControllerMirrorCount = 0;
    gNdsFighterSchedulerLoopP0ButtonTapMask = 0;
    gNdsFighterSchedulerLoopP1ButtonTapMask = 0;
    gNdsFighterSchedulerLoopP0LastStickX = 0;
    gNdsFighterSchedulerLoopP1LastStickX = 0;
    gNdsFighterSchedulerLoopP0FrameCount = 0;
    gNdsFighterSchedulerLoopP1FrameCount = 0;
    gNdsFighterSchedulerLoopP0Completed = 0;
    gNdsFighterSchedulerLoopP1Completed = 0;
    gNdsFighterSchedulerLoopP0StatusVisitMask = 0;
    gNdsFighterSchedulerLoopP1StatusVisitMask = 0;
    gNdsFighterSchedulerLoopP0TransitionMask = 0;
    gNdsFighterSchedulerLoopP1TransitionMask = 0;
    gNdsFighterSchedulerLoopP0WaitVisitCount = 0;
    gNdsFighterSchedulerLoopP1WaitVisitCount = 0;
    gNdsFighterSchedulerLoopP0WalkVisitCount = 0;
    gNdsFighterSchedulerLoopP1WalkVisitCount = 0;
    gNdsFighterSchedulerLoopP0DashVisitCount = 0;
    gNdsFighterSchedulerLoopP1DashVisitCount = 0;
    gNdsFighterSchedulerLoopP0RunVisitCount = 0;
    gNdsFighterSchedulerLoopP1RunVisitCount = 0;
    gNdsFighterSchedulerLoopP0RunBrakeVisitCount = 0;
    gNdsFighterSchedulerLoopP1RunBrakeVisitCount = 0;
    gNdsFighterSchedulerLoopP0KneeBendVisitCount = 0;
    gNdsFighterSchedulerLoopP1KneeBendVisitCount = 0;
    gNdsFighterSchedulerLoopP0JumpVisitCount = 0;
    gNdsFighterSchedulerLoopP1JumpVisitCount = 0;
    gNdsFighterSchedulerLoopP0FallVisitCount = 0;
    gNdsFighterSchedulerLoopP1FallVisitCount = 0;
    gNdsFighterSchedulerLoopP0LandingVisitCount = 0;
    gNdsFighterSchedulerLoopP1LandingVisitCount = 0;
    gNdsFighterSchedulerLoopP0StatusStart = 0xffffffffu;
    gNdsFighterSchedulerLoopP1StatusStart = 0xffffffffu;
    gNdsFighterSchedulerLoopP0MotionStart = 0xffffffffu;
    gNdsFighterSchedulerLoopP1MotionStart = 0xffffffffu;
    gNdsFighterSchedulerLoopP0StatusFinal = 0xffffffffu;
    gNdsFighterSchedulerLoopP1StatusFinal = 0xffffffffu;
    gNdsFighterSchedulerLoopP0MotionFinal = 0xffffffffu;
    gNdsFighterSchedulerLoopP1MotionFinal = 0xffffffffu;
    gNdsFighterSchedulerLoopP0GAFinal = 0xffffffffu;
    gNdsFighterSchedulerLoopP1GAFinal = 0xffffffffu;
    gNdsFighterSchedulerLoopP0FloorYMilli = 0;
    gNdsFighterSchedulerLoopP1FloorYMilli = 0;
    gNdsFighterSchedulerLoopP0RootXStartMilli = 0;
    gNdsFighterSchedulerLoopP1RootXStartMilli = 0;
    gNdsFighterSchedulerLoopP0RootXFinalMilli = 0;
    gNdsFighterSchedulerLoopP1RootXFinalMilli = 0;
    gNdsFighterSchedulerLoopP0RootDeltaXMilli = 0;
    gNdsFighterSchedulerLoopP1RootDeltaXMilli = 0;
    gNdsFighterSchedulerLoopP0RootYFinalMilli = 0;
    gNdsFighterSchedulerLoopP1RootYFinalMilli = 0;
    gNdsFighterSchedulerLoopP0RootRiseMilli = 0;
    gNdsFighterSchedulerLoopP1RootRiseMilli = 0;
    gNdsFighterSchedulerLoopP0RootDirectionOK = 0;
    gNdsFighterSchedulerLoopP1RootDirectionOK = 0;
    gNdsFighterSchedulerLoopP0FloorOK = 0;
    gNdsFighterSchedulerLoopP1FloorOK = 0;
    gNdsFighterSchedulerLoopP0GroundVelFinalMilli = 0;
    gNdsFighterSchedulerLoopP1GroundVelFinalMilli = 0;
    gNdsFighterSchedulerLoopP0AirVelXFinalMilli = 0;
    gNdsFighterSchedulerLoopP1AirVelXFinalMilli = 0;
    gNdsFighterSchedulerLoopP0AirVelYFinalMilli = 0;
    gNdsFighterSchedulerLoopP1AirVelYFinalMilli = 0;
    gNdsFighterSchedulerLoopP0UpdateCount = 0;
    gNdsFighterSchedulerLoopP1UpdateCount = 0;
    gNdsFighterSchedulerLoopP0InterruptCount = 0;
    gNdsFighterSchedulerLoopP1InterruptCount = 0;
    gNdsFighterSchedulerLoopP0PhysicsCount = 0;
    gNdsFighterSchedulerLoopP1PhysicsCount = 0;
    gNdsFighterSchedulerLoopP0IntegrateCount = 0;
    gNdsFighterSchedulerLoopP1IntegrateCount = 0;
    gNdsFighterSchedulerLoopP0MapCount = 0;
    gNdsFighterSchedulerLoopP1MapCount = 0;
    gNdsFighterSchedulerLoopFallDetectCount = 0;
    gNdsFighterSchedulerLoopLandingDetectCount = 0;
    gNdsFighterSchedulerLoopSetGroundCount = 0;
    gNdsFighterSchedulerLoopSetAirCount = 0;
    gNdsFighterSchedulerLoopWaitSetStatusCount = 0;
    gNdsFighterSchedulerLoopRunBrakeEndCount = 0;
    gNdsFighterSchedulerLoopJumpAnimEndCount = 0;
    gNdsFighterSchedulerLoopLandingEndCount = 0;
    gNdsFighterSchedulerLoopDeferredInterruptCheckCount = 0;
    gNdsFighterSchedulerLoopUnexpectedStatusCount = 0;
    gNdsFighterSchedulerLoopDeniedStatusCount = 0;
    gNdsFighterSchedulerLoopDisplayProbeCount = 0;
    gNdsFighterSchedulerLoopGameplayUpdateCount = 0;
    gNdsFighterSchedulerLoopDrawCallCount = 0;
    gNdsFighterSchedulerLoopMatrixCallCount = 0;
    gNdsFighterSchedulerLoopRootYDriftCount = 0;
    gNdsFighterSchedulerLoopGADriftCount = 0;
    ndsControllerPlaybackReset();
    gNdsControllerPollCount = 0;
    gNdsFighterMarioFoxControllerLoopResult = 0;
    gNdsFighterMarioFoxControllerLoopSafeResult = 0;
    gNdsFighterMarioFoxControllerLoopMask = 0;
    gNdsFighterMarioFoxControllerLoopDeferredMask = 0;
    gNdsFighterMarioFoxControllerLoopCount = 0;
    gNdsFighterControllerLoopPrepared = 0;
    gNdsFighterControllerLoopFrameMax = 0;
    gNdsFighterControllerLoopUpdateMax = 0;
    gNdsFighterControllerLoopTaskmanUpdateCount = 0;
    gNdsFighterControllerLoopVSBattleUpdateCount = 0;
    gNdsFighterControllerLoopBaseVSBattleUpdateCount = 0;
    gNdsFighterControllerLoopSchedulerUpdateCount = 0;
    gNdsFighterControllerLoopSYReadCount = 0;
    gNdsFighterControllerLoopSYUpdateCount = 0;
    gNdsFighterControllerLoopGObjCountBefore = 0;
    gNdsFighterControllerLoopGObjCountAfter = 0;
    gNdsFighterControllerLoopGObjDelta = 0;
    gNdsFighterControllerLoopP0ProcessAttachCount = 0;
    gNdsFighterControllerLoopP1ProcessAttachCount = 0;
    gNdsFighterControllerLoopProcessAttachEscapeCount = 0;
    gNdsFighterControllerLoopP0GObjProcessRunCount = 0;
    gNdsFighterControllerLoopP1GObjProcessRunCount = 0;
    gNdsFighterControllerLoopP0ProcCallbackCount = 0;
    gNdsFighterControllerLoopP1ProcCallbackCount = 0;
    gNdsFighterControllerLoopP0PlaybackApplyCount = 0;
    gNdsFighterControllerLoopP1PlaybackApplyCount = 0;
    gNdsFighterControllerLoopP0ControllerToFTInputCount = 0;
    gNdsFighterControllerLoopP1ControllerToFTInputCount = 0;
    gNdsFighterControllerLoopP0DirectFTInputWriteCount = 0;
    gNdsFighterControllerLoopP1DirectFTInputWriteCount = 0;
    gNdsFighterControllerLoopP0ButtonTapMask = 0;
    gNdsFighterControllerLoopP1ButtonTapMask = 0;
    gNdsFighterControllerLoopP0ButtonHoldMask = 0;
    gNdsFighterControllerLoopP1ButtonHoldMask = 0;
    gNdsFighterControllerLoopP0ButtonReleaseMask = 0;
    gNdsFighterControllerLoopP1ButtonReleaseMask = 0;
    gNdsFighterControllerLoopP0LastStickX = 0;
    gNdsFighterControllerLoopP1LastStickX = 0;
    gNdsFighterControllerLoopP0LastStickY = 0;
    gNdsFighterControllerLoopP1LastStickY = 0;
    gNdsFighterControllerLoopP0TapStickXMin = 0xffffffffu;
    gNdsFighterControllerLoopP1TapStickXMin = 0xffffffffu;
    gNdsFighterControllerLoopP0TapStickYMin = 0xffffffffu;
    gNdsFighterControllerLoopP1TapStickYMin = 0xffffffffu;
    gNdsFighterControllerLoopP0DashTapEligibleCount = 0;
    gNdsFighterControllerLoopP1DashTapEligibleCount = 0;
    gNdsFighterControllerLoopP0JumpButtonTapCount = 0;
    gNdsFighterControllerLoopP1JumpButtonTapCount = 0;
    gNdsFighterControllerLoopP0FrameCount = 0;
    gNdsFighterControllerLoopP1FrameCount = 0;
    gNdsFighterControllerLoopP0Completed = 0;
    gNdsFighterControllerLoopP1Completed = 0;
    gNdsFighterControllerLoopP0StatusVisitMask = 0;
    gNdsFighterControllerLoopP1StatusVisitMask = 0;
    gNdsFighterControllerLoopP0TransitionMask = 0;
    gNdsFighterControllerLoopP1TransitionMask = 0;
    gNdsFighterControllerLoopP0WaitVisitCount = 0;
    gNdsFighterControllerLoopP1WaitVisitCount = 0;
    gNdsFighterControllerLoopP0WalkVisitCount = 0;
    gNdsFighterControllerLoopP1WalkVisitCount = 0;
    gNdsFighterControllerLoopP0DashVisitCount = 0;
    gNdsFighterControllerLoopP1DashVisitCount = 0;
    gNdsFighterControllerLoopP0RunVisitCount = 0;
    gNdsFighterControllerLoopP1RunVisitCount = 0;
    gNdsFighterControllerLoopP0RunBrakeVisitCount = 0;
    gNdsFighterControllerLoopP1RunBrakeVisitCount = 0;
    gNdsFighterControllerLoopP0KneeBendVisitCount = 0;
    gNdsFighterControllerLoopP1KneeBendVisitCount = 0;
    gNdsFighterControllerLoopP0JumpVisitCount = 0;
    gNdsFighterControllerLoopP1JumpVisitCount = 0;
    gNdsFighterControllerLoopP0FallVisitCount = 0;
    gNdsFighterControllerLoopP1FallVisitCount = 0;
    gNdsFighterControllerLoopP0LandingVisitCount = 0;
    gNdsFighterControllerLoopP1LandingVisitCount = 0;
    gNdsFighterControllerLoopP0StatusStart = 0xffffffffu;
    gNdsFighterControllerLoopP1StatusStart = 0xffffffffu;
    gNdsFighterControllerLoopP0MotionStart = 0xffffffffu;
    gNdsFighterControllerLoopP1MotionStart = 0xffffffffu;
    gNdsFighterControllerLoopP0StatusFinal = 0xffffffffu;
    gNdsFighterControllerLoopP1StatusFinal = 0xffffffffu;
    gNdsFighterControllerLoopP0MotionFinal = 0xffffffffu;
    gNdsFighterControllerLoopP1MotionFinal = 0xffffffffu;
    gNdsFighterControllerLoopP0GAFinal = 0xffffffffu;
    gNdsFighterControllerLoopP1GAFinal = 0xffffffffu;
    gNdsFighterControllerLoopP0FloorYMilli = 0;
    gNdsFighterControllerLoopP1FloorYMilli = 0;
    gNdsFighterControllerLoopP0RootXStartMilli = 0;
    gNdsFighterControllerLoopP1RootXStartMilli = 0;
    gNdsFighterControllerLoopP0RootXFinalMilli = 0;
    gNdsFighterControllerLoopP1RootXFinalMilli = 0;
    gNdsFighterControllerLoopP0RootDeltaXMilli = 0;
    gNdsFighterControllerLoopP1RootDeltaXMilli = 0;
    gNdsFighterControllerLoopP0RootYFinalMilli = 0;
    gNdsFighterControllerLoopP1RootYFinalMilli = 0;
    gNdsFighterControllerLoopP0RootRiseMilli = 0;
    gNdsFighterControllerLoopP1RootRiseMilli = 0;
    gNdsFighterControllerLoopP0RootDirectionOK = 0;
    gNdsFighterControllerLoopP1RootDirectionOK = 0;
    gNdsFighterControllerLoopP0FloorOK = 0;
    gNdsFighterControllerLoopP1FloorOK = 0;
    gNdsFighterControllerLoopP0GroundVelFinalMilli = 0;
    gNdsFighterControllerLoopP1GroundVelFinalMilli = 0;
    gNdsFighterControllerLoopP0AirVelXFinalMilli = 0;
    gNdsFighterControllerLoopP1AirVelXFinalMilli = 0;
    gNdsFighterControllerLoopP0AirVelYFinalMilli = 0;
    gNdsFighterControllerLoopP1AirVelYFinalMilli = 0;
    gNdsFighterControllerLoopP0UpdateCount = 0;
    gNdsFighterControllerLoopP1UpdateCount = 0;
    gNdsFighterControllerLoopP0InterruptCount = 0;
    gNdsFighterControllerLoopP1InterruptCount = 0;
    gNdsFighterControllerLoopP0PhysicsCount = 0;
    gNdsFighterControllerLoopP1PhysicsCount = 0;
    gNdsFighterControllerLoopP0IntegrateCount = 0;
    gNdsFighterControllerLoopP1IntegrateCount = 0;
    gNdsFighterControllerLoopP0MapCount = 0;
    gNdsFighterControllerLoopP1MapCount = 0;
    gNdsFighterControllerLoopFallDetectCount = 0;
    gNdsFighterControllerLoopLandingDetectCount = 0;
    gNdsFighterControllerLoopSetGroundCount = 0;
    gNdsFighterControllerLoopSetAirCount = 0;
    gNdsFighterControllerLoopWaitSetStatusCount = 0;
    gNdsFighterControllerLoopRunBrakeEndCount = 0;
    gNdsFighterControllerLoopJumpAnimEndCount = 0;
    gNdsFighterControllerLoopLandingEndCount = 0;
    gNdsFighterControllerLoopDeferredInterruptCheckCount = 0;
    gNdsFighterControllerLoopUnexpectedStatusCount = 0;
    gNdsFighterControllerLoopDeniedStatusCount = 0;
    gNdsFighterControllerLoopDisplayProbeCount = 0;
    gNdsFighterControllerLoopGameplayUpdateCount = 0;
    gNdsFighterControllerLoopDrawCallCount = 0;
    gNdsFighterControllerLoopMatrixCallCount = 0;
    gNdsFighterControllerLoopRootYDriftCount = 0;
    gNdsFighterControllerLoopGADriftCount = 0;
    gNdsFighterMarioFoxPreviewLoopResult = 0;
    gNdsFighterMarioFoxPreviewLoopSafeResult = 0;
    gNdsFighterMarioFoxPreviewLoopMask = 0;
    gNdsFighterMarioFoxPreviewLoopDeferredMask = 0;
    gNdsFighterMarioFoxPreviewLoopCount = 0;
    gNdsFighterPreviewLoopPrepared = 0;
    gNdsFighterPreviewLoopFrameMax = 0;
    gNdsFighterPreviewLoopUpdateMax = 0;
    gNdsFighterPreviewLoopTaskmanUpdateCount = 0;
    gNdsFighterPreviewLoopVSBattleUpdateCount = 0;
    gNdsFighterPreviewLoopBaseVSBattleUpdateCount = 0;
    gNdsFighterPreviewLoopSchedulerUpdateCount = 0;
    gNdsFighterPreviewLoopSYReadCount = 0;
    gNdsFighterPreviewLoopSYUpdateCount = 0;
    gNdsFighterPreviewLoopGObjCountBefore = 0;
    gNdsFighterPreviewLoopGObjCountAfter = 0;
    gNdsFighterPreviewLoopGObjDelta = 0;
    gNdsFighterPreviewLoopP0ProcessAttachCount = 0;
    gNdsFighterPreviewLoopP1ProcessAttachCount = 0;
    gNdsFighterPreviewLoopProcessAttachEscapeCount = 0;
    gNdsFighterPreviewLoopP0GObjProcessRunCount = 0;
    gNdsFighterPreviewLoopP1GObjProcessRunCount = 0;
    gNdsFighterPreviewLoopP0ProcCallbackCount = 0;
    gNdsFighterPreviewLoopP1ProcCallbackCount = 0;
    gNdsFighterPreviewLoopP0PlaybackApplyCount = 0;
    gNdsFighterPreviewLoopP1PlaybackApplyCount = 0;
    gNdsFighterPreviewLoopP0ControllerToFTInputCount = 0;
    gNdsFighterPreviewLoopP1ControllerToFTInputCount = 0;
    gNdsFighterPreviewLoopP0DirectFTInputWriteCount = 0;
    gNdsFighterPreviewLoopP1DirectFTInputWriteCount = 0;
    gNdsFighterPreviewLoopP0ButtonTapMask = 0;
    gNdsFighterPreviewLoopP1ButtonTapMask = 0;
    gNdsFighterPreviewLoopP0ButtonHoldMask = 0;
    gNdsFighterPreviewLoopP1ButtonHoldMask = 0;
    gNdsFighterPreviewLoopP0LastStickX = 0;
    gNdsFighterPreviewLoopP1LastStickX = 0;
    gNdsFighterPreviewLoopP0LastStickY = 0;
    gNdsFighterPreviewLoopP1LastStickY = 0;
    gNdsFighterPreviewLoopP0DashTapEligibleCount = 0;
    gNdsFighterPreviewLoopP1DashTapEligibleCount = 0;
    gNdsFighterPreviewLoopP0JumpButtonTapCount = 0;
    gNdsFighterPreviewLoopP1JumpButtonTapCount = 0;
    gNdsFighterPreviewLoopP0FrameCount = 0;
    gNdsFighterPreviewLoopP1FrameCount = 0;
    gNdsFighterPreviewLoopP0Completed = 0;
    gNdsFighterPreviewLoopP1Completed = 0;
    gNdsFighterPreviewLoopP0StatusVisitMask = 0;
    gNdsFighterPreviewLoopP1StatusVisitMask = 0;
    gNdsFighterPreviewLoopP0TransitionMask = 0;
    gNdsFighterPreviewLoopP1TransitionMask = 0;
    gNdsFighterPreviewLoopP0StatusStart = 0xffffffffu;
    gNdsFighterPreviewLoopP1StatusStart = 0xffffffffu;
    gNdsFighterPreviewLoopP0MotionStart = 0xffffffffu;
    gNdsFighterPreviewLoopP1MotionStart = 0xffffffffu;
    gNdsFighterPreviewLoopP0StatusFinal = 0xffffffffu;
    gNdsFighterPreviewLoopP1StatusFinal = 0xffffffffu;
    gNdsFighterPreviewLoopP0MotionFinal = 0xffffffffu;
    gNdsFighterPreviewLoopP1MotionFinal = 0xffffffffu;
    gNdsFighterPreviewLoopP0GAFinal = 0xffffffffu;
    gNdsFighterPreviewLoopP1GAFinal = 0xffffffffu;
    gNdsFighterPreviewLoopP0RootXStartMilli = 0;
    gNdsFighterPreviewLoopP1RootXStartMilli = 0;
    gNdsFighterPreviewLoopP0RootDeltaXMilli = 0;
    gNdsFighterPreviewLoopP1RootDeltaXMilli = 0;
    gNdsFighterPreviewLoopP0RootRiseMilli = 0;
    gNdsFighterPreviewLoopP1RootRiseMilli = 0;
    gNdsFighterPreviewLoopP0RootYFinalMilli = 0;
    gNdsFighterPreviewLoopP1RootYFinalMilli = 0;
    gNdsFighterPreviewLoopP0FloorYMilli = 0;
    gNdsFighterPreviewLoopP1FloorYMilli = 0;
    gNdsFighterPreviewLoopP0RootDirectionOK = 0;
    gNdsFighterPreviewLoopP1RootDirectionOK = 0;
    gNdsFighterPreviewLoopP0FloorOK = 0;
    gNdsFighterPreviewLoopP1FloorOK = 0;
    gNdsFighterPreviewLoopP0InterruptCount = 0;
    gNdsFighterPreviewLoopP1InterruptCount = 0;
    gNdsFighterPreviewLoopP0PhysicsCount = 0;
    gNdsFighterPreviewLoopP1PhysicsCount = 0;
    gNdsFighterPreviewLoopP0IntegrateCount = 0;
    gNdsFighterPreviewLoopP1IntegrateCount = 0;
    gNdsFighterPreviewLoopP0MapCount = 0;
    gNdsFighterPreviewLoopP1MapCount = 0;
    gNdsFighterPreviewLoopPreviewWidth = 0;
    gNdsFighterPreviewLoopPreviewHeight = 0;
    gNdsFighterPreviewLoopPreviewPitch = 0;
    gNdsFighterPreviewLoopPreviewReady = 0;
    gNdsFighterPreviewLoopPreviewCommitBefore = 0;
    gNdsFighterPreviewLoopPreviewCommitAfter = 0;
    gNdsFighterPreviewLoopPreviewCommitDelta = 0;
    gNdsFighterPreviewLoopDrawFrameCount = 0;
    gNdsFighterPreviewLoopDisplayCallbackCount = 0;
    gNdsFighterPreviewLoopP0DisplayCallbackCount = 0;
    gNdsFighterPreviewLoopP1DisplayCallbackCount = 0;
    gNdsFighterPreviewLoopP0CandidateCount = 0;
    gNdsFighterPreviewLoopP1CandidateCount = 0;
    gNdsFighterPreviewLoopP0DrawnDObjCount = 0;
    gNdsFighterPreviewLoopP1DrawnDObjCount = 0;
    gNdsFighterPreviewLoopP0PixelCount = 0;
    gNdsFighterPreviewLoopP1PixelCount = 0;
    gNdsFighterPreviewLoopTotalPixelCount = 0;
    gNdsFighterPreviewLoopP0ColorChecksum = 0;
    gNdsFighterPreviewLoopP1ColorChecksum = 0;
    gNdsFighterPreviewLoopP0ScreenXStart = 0;
    gNdsFighterPreviewLoopP1ScreenXStart = 0;
    gNdsFighterPreviewLoopP0ScreenXFinal = 0;
    gNdsFighterPreviewLoopP1ScreenXFinal = 0;
    gNdsFighterPreviewLoopP0ScreenXDelta = 0;
    gNdsFighterPreviewLoopP1ScreenXDelta = 0;
    gNdsFighterPreviewLoopP0ScreenYFloor = 0;
    gNdsFighterPreviewLoopP1ScreenYFloor = 0;
    gNdsFighterPreviewLoopP0ScreenYMin = 0x7fffffff;
    gNdsFighterPreviewLoopP1ScreenYMin = 0x7fffffff;
    gNdsFighterPreviewLoopP0ScreenRise = 0;
    gNdsFighterPreviewLoopP1ScreenRise = 0;
    gNdsFighterPreviewLoopFallDetectCount = 0;
    gNdsFighterPreviewLoopLandingDetectCount = 0;
    gNdsFighterPreviewLoopSetGroundCount = 0;
    gNdsFighterPreviewLoopSetAirCount = 0;
    gNdsFighterPreviewLoopWaitSetStatusCount = 0;
    gNdsFighterPreviewLoopRunBrakeEndCount = 0;
    gNdsFighterPreviewLoopJumpAnimEndCount = 0;
    gNdsFighterPreviewLoopLandingEndCount = 0;
    gNdsFighterPreviewLoopDeferredInterruptCheckCount = 0;
    gNdsFighterPreviewLoopUnexpectedStatusCount = 0;
    gNdsFighterPreviewLoopDeniedStatusCount = 0;
    gNdsFighterPreviewLoopDisplayProbeCount = 0;
    gNdsFighterPreviewLoopGameplayUpdateCount = 0;
    gNdsFighterPreviewLoopDrawCallCount = 0;
    gNdsFighterPreviewLoopMatrixCallCount = 0;
    gNdsFighterPreviewLoopRootYDriftCount = 0;
    gNdsFighterPreviewLoopGADriftCount = 0;
    gNdsFighterMarioFoxGCRunAllLoopResult = 0;
    gNdsFighterMarioFoxGCRunAllLoopSafeResult = 0;
    gNdsFighterMarioFoxGCRunAllLoopMask = 0;
    gNdsFighterMarioFoxGCRunAllLoopDeferredMask = 0;
    gNdsFighterMarioFoxGCRunAllLoopCount = 0;
    gNdsFighterGCRunAllLoopPrepared = 0;
    gNdsFighterGCRunAllLoopFrameMax = 0;
    gNdsFighterGCRunAllLoopUpdateMax = 0;
    gNdsFighterGCRunAllLoopTaskmanUpdateCount = 0;
    gNdsFighterGCRunAllLoopVSBattleUpdateCount = 0;
    gNdsFighterGCRunAllLoopBaseVSBattleUpdateCount = 0;
    gNdsFighterGCRunAllLoopRunAllCount = 0;
    gNdsFighterGCRunAllLoopSYReadCount = 0;
    gNdsFighterGCRunAllLoopSYUpdateCount = 0;
    gNdsFighterGCRunAllLoopGObjCountBefore = 0;
    gNdsFighterGCRunAllLoopGObjCountAfter = 0;
    gNdsFighterGCRunAllLoopGObjDelta = 0;
    gNdsFighterGCRunAllLoopOldProcessPauseCount = 0;
    gNdsFighterGCRunAllLoopNonTargetGObjVisitCount = 0;
    gNdsFighterGCRunAllLoopNonTargetProcessPauseCount = 0;
    gNdsFighterGCRunAllLoopTargetProcessPreserveCount = 0;
    gNdsFighterGCRunAllLoopP0ProcessAttachCount = 0;
    gNdsFighterGCRunAllLoopP1ProcessAttachCount = 0;
    gNdsFighterGCRunAllLoopProcessAttachEscapeCount = 0;
    gNdsFighterGCRunAllLoopP0GObjProcessRunCount = 0;
    gNdsFighterGCRunAllLoopP1GObjProcessRunCount = 0;
    gNdsFighterGCRunAllLoopP0ProcCallbackCount = 0;
    gNdsFighterGCRunAllLoopP1ProcCallbackCount = 0;
    gNdsFighterGCRunAllLoopP0PlaybackApplyCount = 0;
    gNdsFighterGCRunAllLoopP1PlaybackApplyCount = 0;
    gNdsFighterGCRunAllLoopP0ControllerToFTInputCount = 0;
    gNdsFighterGCRunAllLoopP1ControllerToFTInputCount = 0;
    gNdsFighterGCRunAllLoopP0DirectFTInputWriteCount = 0;
    gNdsFighterGCRunAllLoopP1DirectFTInputWriteCount = 0;
    gNdsFighterGCRunAllLoopP0ButtonTapMask = 0;
    gNdsFighterGCRunAllLoopP1ButtonTapMask = 0;
    gNdsFighterGCRunAllLoopP0ButtonHoldMask = 0;
    gNdsFighterGCRunAllLoopP1ButtonHoldMask = 0;
    gNdsFighterGCRunAllLoopP0LastStickX = 0;
    gNdsFighterGCRunAllLoopP1LastStickX = 0;
    gNdsFighterGCRunAllLoopP0LastStickY = 0;
    gNdsFighterGCRunAllLoopP1LastStickY = 0;
    gNdsFighterGCRunAllLoopP0DashTapEligibleCount = 0;
    gNdsFighterGCRunAllLoopP1DashTapEligibleCount = 0;
    gNdsFighterGCRunAllLoopP0JumpButtonTapCount = 0;
    gNdsFighterGCRunAllLoopP1JumpButtonTapCount = 0;
    gNdsFighterGCRunAllLoopP0FrameCount = 0;
    gNdsFighterGCRunAllLoopP1FrameCount = 0;
    gNdsFighterGCRunAllLoopP0Completed = 0;
    gNdsFighterGCRunAllLoopP1Completed = 0;
    gNdsFighterGCRunAllLoopP0StatusVisitMask = 0;
    gNdsFighterGCRunAllLoopP1StatusVisitMask = 0;
    gNdsFighterGCRunAllLoopP0TransitionMask = 0;
    gNdsFighterGCRunAllLoopP1TransitionMask = 0;
    gNdsFighterGCRunAllLoopP0WaitVisitCount = 0;
    gNdsFighterGCRunAllLoopP1WaitVisitCount = 0;
    gNdsFighterGCRunAllLoopP0WalkVisitCount = 0;
    gNdsFighterGCRunAllLoopP1WalkVisitCount = 0;
    gNdsFighterGCRunAllLoopP0DashVisitCount = 0;
    gNdsFighterGCRunAllLoopP1DashVisitCount = 0;
    gNdsFighterGCRunAllLoopP0RunVisitCount = 0;
    gNdsFighterGCRunAllLoopP1RunVisitCount = 0;
    gNdsFighterGCRunAllLoopP0RunBrakeVisitCount = 0;
    gNdsFighterGCRunAllLoopP1RunBrakeVisitCount = 0;
    gNdsFighterGCRunAllLoopP0KneeBendVisitCount = 0;
    gNdsFighterGCRunAllLoopP1KneeBendVisitCount = 0;
    gNdsFighterGCRunAllLoopP0JumpVisitCount = 0;
    gNdsFighterGCRunAllLoopP1JumpVisitCount = 0;
    gNdsFighterGCRunAllLoopP0FallVisitCount = 0;
    gNdsFighterGCRunAllLoopP1FallVisitCount = 0;
    gNdsFighterGCRunAllLoopP0LandingVisitCount = 0;
    gNdsFighterGCRunAllLoopP1LandingVisitCount = 0;
    gNdsFighterGCRunAllLoopP0StatusStart = 0xffffffffu;
    gNdsFighterGCRunAllLoopP1StatusStart = 0xffffffffu;
    gNdsFighterGCRunAllLoopP0MotionStart = 0xffffffffu;
    gNdsFighterGCRunAllLoopP1MotionStart = 0xffffffffu;
    gNdsFighterGCRunAllLoopP0StatusFinal = 0xffffffffu;
    gNdsFighterGCRunAllLoopP1StatusFinal = 0xffffffffu;
    gNdsFighterGCRunAllLoopP0MotionFinal = 0xffffffffu;
    gNdsFighterGCRunAllLoopP1MotionFinal = 0xffffffffu;
    gNdsFighterGCRunAllLoopP0GAFinal = 0xffffffffu;
    gNdsFighterGCRunAllLoopP1GAFinal = 0xffffffffu;
    gNdsFighterGCRunAllLoopP0RootXStartMilli = 0;
    gNdsFighterGCRunAllLoopP1RootXStartMilli = 0;
    gNdsFighterGCRunAllLoopP0RootDeltaXMilli = 0;
    gNdsFighterGCRunAllLoopP1RootDeltaXMilli = 0;
    gNdsFighterGCRunAllLoopP0RootRiseMilli = 0;
    gNdsFighterGCRunAllLoopP1RootRiseMilli = 0;
    gNdsFighterGCRunAllLoopP0RootYFinalMilli = 0;
    gNdsFighterGCRunAllLoopP1RootYFinalMilli = 0;
    gNdsFighterGCRunAllLoopP0FloorYMilli = 0;
    gNdsFighterGCRunAllLoopP1FloorYMilli = 0;
    gNdsFighterGCRunAllLoopP0RootDirectionOK = 0;
    gNdsFighterGCRunAllLoopP1RootDirectionOK = 0;
    gNdsFighterGCRunAllLoopP0FloorOK = 0;
    gNdsFighterGCRunAllLoopP1FloorOK = 0;
    gNdsFighterGCRunAllLoopP0InterruptCount = 0;
    gNdsFighterGCRunAllLoopP1InterruptCount = 0;
    gNdsFighterGCRunAllLoopP0PhysicsCount = 0;
    gNdsFighterGCRunAllLoopP1PhysicsCount = 0;
    gNdsFighterGCRunAllLoopP0IntegrateCount = 0;
    gNdsFighterGCRunAllLoopP1IntegrateCount = 0;
    gNdsFighterGCRunAllLoopP0MapCount = 0;
    gNdsFighterGCRunAllLoopP1MapCount = 0;
    gNdsFighterGCRunAllLoopPreviewWidth = 0;
    gNdsFighterGCRunAllLoopPreviewHeight = 0;
    gNdsFighterGCRunAllLoopPreviewPitch = 0;
    gNdsFighterGCRunAllLoopPreviewReady = 0;
    gNdsFighterGCRunAllLoopPreviewCommitBefore = 0;
    gNdsFighterGCRunAllLoopPreviewCommitAfter = 0;
    gNdsFighterGCRunAllLoopPreviewCommitDelta = 0;
    gNdsFighterGCRunAllLoopDrawFrameCount = 0;
    gNdsFighterGCRunAllLoopDisplayCallbackCount = 0;
    gNdsFighterGCRunAllLoopP0DisplayCallbackCount = 0;
    gNdsFighterGCRunAllLoopP1DisplayCallbackCount = 0;
    gNdsFighterGCRunAllLoopP0CandidateCount = 0;
    gNdsFighterGCRunAllLoopP1CandidateCount = 0;
    gNdsFighterGCRunAllLoopP0DrawnDObjCount = 0;
    gNdsFighterGCRunAllLoopP1DrawnDObjCount = 0;
    gNdsFighterGCRunAllLoopP0PixelCount = 0;
    gNdsFighterGCRunAllLoopP1PixelCount = 0;
    gNdsFighterGCRunAllLoopTotalPixelCount = 0;
    gNdsFighterGCRunAllLoopP0ColorChecksum = 0;
    gNdsFighterGCRunAllLoopP1ColorChecksum = 0;
    gNdsFighterGCRunAllLoopP0ScreenXStart = 0;
    gNdsFighterGCRunAllLoopP1ScreenXStart = 0;
    gNdsFighterGCRunAllLoopP0ScreenXFinal = 0;
    gNdsFighterGCRunAllLoopP1ScreenXFinal = 0;
    gNdsFighterGCRunAllLoopP0ScreenXDelta = 0;
    gNdsFighterGCRunAllLoopP1ScreenXDelta = 0;
    gNdsFighterGCRunAllLoopP0ScreenYFloor = 0;
    gNdsFighterGCRunAllLoopP1ScreenYFloor = 0;
    gNdsFighterGCRunAllLoopP0ScreenYMin = 0x7fffffff;
    gNdsFighterGCRunAllLoopP1ScreenYMin = 0x7fffffff;
    gNdsFighterGCRunAllLoopP0ScreenRise = 0;
    gNdsFighterGCRunAllLoopP1ScreenRise = 0;
    gNdsFighterGCRunAllLoopFallDetectCount = 0;
    gNdsFighterGCRunAllLoopLandingDetectCount = 0;
    gNdsFighterGCRunAllLoopSetGroundCount = 0;
    gNdsFighterGCRunAllLoopSetAirCount = 0;
    gNdsFighterGCRunAllLoopWaitSetStatusCount = 0;
    gNdsFighterGCRunAllLoopRunBrakeEndCount = 0;
    gNdsFighterGCRunAllLoopJumpAnimEndCount = 0;
    gNdsFighterGCRunAllLoopLandingEndCount = 0;
    gNdsFighterGCRunAllLoopDeferredInterruptCheckCount = 0;
    gNdsFighterGCRunAllLoopUnexpectedStatusCount = 0;
    gNdsFighterGCRunAllLoopDeniedStatusCount = 0;
    gNdsFighterGCRunAllLoopDisplayProbeCount = 0;
    gNdsFighterGCRunAllLoopGameplayUpdateCount = 0;
    gNdsFighterGCRunAllLoopDrawCallCount = 0;
    gNdsFighterGCRunAllLoopMatrixCallCount = 0;
    gNdsFighterGCRunAllLoopRootYDriftCount = 0;
    gNdsFighterGCRunAllLoopGADriftCount = 0;
    gNdsFighterMarioFoxGCDrawAllLoopResult = 0;
    gNdsFighterMarioFoxGCDrawAllLoopSafeResult = 0;
    gNdsFighterMarioFoxGCDrawAllLoopMask = 0;
    gNdsFighterMarioFoxGCDrawAllLoopDeferredMask = 0;
    gNdsFighterMarioFoxGCDrawAllLoopCount = 0;
    gNdsFighterGCDrawAllLoopPrepared = 0;
    gNdsFighterGCDrawAllLoopFrameMax = 0;
    gNdsFighterGCDrawAllLoopUpdateMax = 0;
    gNdsFighterGCDrawAllLoopTaskmanUpdateCount = 0;
    gNdsFighterGCDrawAllLoopVSBattleUpdateCount = 0;
    gNdsFighterGCDrawAllLoopBaseVSBattleUpdateCount = 0;
    gNdsFighterGCDrawAllLoopDrawAllCount = 0;
    gNdsFighterGCDrawAllLoopCameraCallbackCount = 0;
    gNdsFighterGCDrawAllLoopCapturedDisplayCount = 0;
    gNdsFighterGCDrawAllLoopNonTargetDisplayCallbackCount = 0;
    gNdsFighterGCDrawAllLoopRunAllCount = 0;
    gNdsFighterGCDrawAllLoopSYReadCount = 0;
    gNdsFighterGCDrawAllLoopSYUpdateCount = 0;
    gNdsFighterGCDrawAllLoopGObjCountBefore = 0;
    gNdsFighterGCDrawAllLoopGObjCountAfter = 0;
    gNdsFighterGCDrawAllLoopGObjDelta = 0;
    gNdsFighterGCDrawAllLoopOldProcessPauseCount = 0;
    gNdsFighterGCDrawAllLoopNonTargetGObjVisitCount = 0;
    gNdsFighterGCDrawAllLoopNonTargetProcessPauseCount = 0;
    gNdsFighterGCDrawAllLoopTargetProcessPreserveCount = 0;
    gNdsFighterGCDrawAllLoopP0ProcessAttachCount = 0;
    gNdsFighterGCDrawAllLoopP1ProcessAttachCount = 0;
    gNdsFighterGCDrawAllLoopProcessAttachEscapeCount = 0;
    gNdsFighterGCDrawAllLoopP0GObjProcessRunCount = 0;
    gNdsFighterGCDrawAllLoopP1GObjProcessRunCount = 0;
    gNdsFighterGCDrawAllLoopP0ProcCallbackCount = 0;
    gNdsFighterGCDrawAllLoopP1ProcCallbackCount = 0;
    gNdsFighterGCDrawAllLoopP0PlaybackApplyCount = 0;
    gNdsFighterGCDrawAllLoopP1PlaybackApplyCount = 0;
    gNdsFighterGCDrawAllLoopP0ControllerToFTInputCount = 0;
    gNdsFighterGCDrawAllLoopP1ControllerToFTInputCount = 0;
    gNdsFighterGCDrawAllLoopP0DirectFTInputWriteCount = 0;
    gNdsFighterGCDrawAllLoopP1DirectFTInputWriteCount = 0;
    gNdsFighterGCDrawAllLoopP0ButtonTapMask = 0;
    gNdsFighterGCDrawAllLoopP1ButtonTapMask = 0;
    gNdsFighterGCDrawAllLoopP0ButtonHoldMask = 0;
    gNdsFighterGCDrawAllLoopP1ButtonHoldMask = 0;
    gNdsFighterGCDrawAllLoopP0LastStickX = 0;
    gNdsFighterGCDrawAllLoopP1LastStickX = 0;
    gNdsFighterGCDrawAllLoopP0LastStickY = 0;
    gNdsFighterGCDrawAllLoopP1LastStickY = 0;
    gNdsFighterGCDrawAllLoopP0DashTapEligibleCount = 0;
    gNdsFighterGCDrawAllLoopP1DashTapEligibleCount = 0;
    gNdsFighterGCDrawAllLoopP0JumpButtonTapCount = 0;
    gNdsFighterGCDrawAllLoopP1JumpButtonTapCount = 0;
    gNdsFighterGCDrawAllLoopP0FrameCount = 0;
    gNdsFighterGCDrawAllLoopP1FrameCount = 0;
    gNdsFighterGCDrawAllLoopP0Completed = 0;
    gNdsFighterGCDrawAllLoopP1Completed = 0;
    gNdsFighterGCDrawAllLoopP0StatusVisitMask = 0;
    gNdsFighterGCDrawAllLoopP1StatusVisitMask = 0;
    gNdsFighterGCDrawAllLoopP0TransitionMask = 0;
    gNdsFighterGCDrawAllLoopP1TransitionMask = 0;
    gNdsFighterGCDrawAllLoopP0WaitVisitCount = 0;
    gNdsFighterGCDrawAllLoopP1WaitVisitCount = 0;
    gNdsFighterGCDrawAllLoopP0WalkVisitCount = 0;
    gNdsFighterGCDrawAllLoopP1WalkVisitCount = 0;
    gNdsFighterGCDrawAllLoopP0DashVisitCount = 0;
    gNdsFighterGCDrawAllLoopP1DashVisitCount = 0;
    gNdsFighterGCDrawAllLoopP0RunVisitCount = 0;
    gNdsFighterGCDrawAllLoopP1RunVisitCount = 0;
    gNdsFighterGCDrawAllLoopP0RunBrakeVisitCount = 0;
    gNdsFighterGCDrawAllLoopP1RunBrakeVisitCount = 0;
    gNdsFighterGCDrawAllLoopP0KneeBendVisitCount = 0;
    gNdsFighterGCDrawAllLoopP1KneeBendVisitCount = 0;
    gNdsFighterGCDrawAllLoopP0JumpVisitCount = 0;
    gNdsFighterGCDrawAllLoopP1JumpVisitCount = 0;
    gNdsFighterGCDrawAllLoopP0FallVisitCount = 0;
    gNdsFighterGCDrawAllLoopP1FallVisitCount = 0;
    gNdsFighterGCDrawAllLoopP0LandingVisitCount = 0;
    gNdsFighterGCDrawAllLoopP1LandingVisitCount = 0;
    gNdsFighterGCDrawAllLoopP0StatusStart = 0xffffffffu;
    gNdsFighterGCDrawAllLoopP1StatusStart = 0xffffffffu;
    gNdsFighterGCDrawAllLoopP0MotionStart = 0xffffffffu;
    gNdsFighterGCDrawAllLoopP1MotionStart = 0xffffffffu;
    gNdsFighterGCDrawAllLoopP0StatusFinal = 0xffffffffu;
    gNdsFighterGCDrawAllLoopP1StatusFinal = 0xffffffffu;
    gNdsFighterGCDrawAllLoopP0MotionFinal = 0xffffffffu;
    gNdsFighterGCDrawAllLoopP1MotionFinal = 0xffffffffu;
    gNdsFighterGCDrawAllLoopP0GAFinal = 0xffffffffu;
    gNdsFighterGCDrawAllLoopP1GAFinal = 0xffffffffu;
    gNdsFighterGCDrawAllLoopP0RootXStartMilli = 0;
    gNdsFighterGCDrawAllLoopP1RootXStartMilli = 0;
    gNdsFighterGCDrawAllLoopP0RootDeltaXMilli = 0;
    gNdsFighterGCDrawAllLoopP1RootDeltaXMilli = 0;
    gNdsFighterGCDrawAllLoopP0RootRiseMilli = 0;
    gNdsFighterGCDrawAllLoopP1RootRiseMilli = 0;
    gNdsFighterGCDrawAllLoopP0RootYFinalMilli = 0;
    gNdsFighterGCDrawAllLoopP1RootYFinalMilli = 0;
    gNdsFighterGCDrawAllLoopP0FloorYMilli = 0;
    gNdsFighterGCDrawAllLoopP1FloorYMilli = 0;
    gNdsFighterGCDrawAllLoopP0RootDirectionOK = 0;
    gNdsFighterGCDrawAllLoopP1RootDirectionOK = 0;
    gNdsFighterGCDrawAllLoopP0FloorOK = 0;
    gNdsFighterGCDrawAllLoopP1FloorOK = 0;
    gNdsFighterGCDrawAllLoopP0InterruptCount = 0;
    gNdsFighterGCDrawAllLoopP1InterruptCount = 0;
    gNdsFighterGCDrawAllLoopP0PhysicsCount = 0;
    gNdsFighterGCDrawAllLoopP1PhysicsCount = 0;
    gNdsFighterGCDrawAllLoopP0IntegrateCount = 0;
    gNdsFighterGCDrawAllLoopP1IntegrateCount = 0;
    gNdsFighterGCDrawAllLoopP0MapCount = 0;
    gNdsFighterGCDrawAllLoopP1MapCount = 0;
    gNdsFighterGCDrawAllLoopPreviewWidth = 0;
    gNdsFighterGCDrawAllLoopPreviewHeight = 0;
    gNdsFighterGCDrawAllLoopPreviewPitch = 0;
    gNdsFighterGCDrawAllLoopPreviewReady = 0;
    gNdsFighterGCDrawAllLoopPreviewCommitBefore = 0;
    gNdsFighterGCDrawAllLoopPreviewCommitAfter = 0;
    gNdsFighterGCDrawAllLoopPreviewCommitDelta = 0;
    gNdsFighterGCDrawAllLoopDrawFrameCount = 0;
    gNdsFighterGCDrawAllLoopDisplayCallbackCount = 0;
    gNdsFighterGCDrawAllLoopP0DisplayCallbackCount = 0;
    gNdsFighterGCDrawAllLoopP1DisplayCallbackCount = 0;
    gNdsFighterGCDrawAllLoopP0CandidateCount = 0;
    gNdsFighterGCDrawAllLoopP1CandidateCount = 0;
    gNdsFighterGCDrawAllLoopP0DrawnDObjCount = 0;
    gNdsFighterGCDrawAllLoopP1DrawnDObjCount = 0;
    gNdsFighterGCDrawAllLoopP0PixelCount = 0;
    gNdsFighterGCDrawAllLoopP1PixelCount = 0;
    gNdsFighterGCDrawAllLoopTotalPixelCount = 0;
    gNdsFighterGCDrawAllLoopP0ColorChecksum = 0;
    gNdsFighterGCDrawAllLoopP1ColorChecksum = 0;
    gNdsFighterGCDrawAllLoopP0ScreenXStart = 0;
    gNdsFighterGCDrawAllLoopP1ScreenXStart = 0;
    gNdsFighterGCDrawAllLoopP0ScreenXFinal = 0;
    gNdsFighterGCDrawAllLoopP1ScreenXFinal = 0;
    gNdsFighterGCDrawAllLoopP0ScreenXDelta = 0;
    gNdsFighterGCDrawAllLoopP1ScreenXDelta = 0;
    gNdsFighterGCDrawAllLoopP0ScreenYFloor = 0;
    gNdsFighterGCDrawAllLoopP1ScreenYFloor = 0;
    gNdsFighterGCDrawAllLoopP0ScreenYMin = 0x7fffffff;
    gNdsFighterGCDrawAllLoopP1ScreenYMin = 0x7fffffff;
    gNdsFighterGCDrawAllLoopP0ScreenRise = 0;
    gNdsFighterGCDrawAllLoopP1ScreenRise = 0;
    gNdsFighterGCDrawAllLoopFallDetectCount = 0;
    gNdsFighterGCDrawAllLoopLandingDetectCount = 0;
    gNdsFighterGCDrawAllLoopSetGroundCount = 0;
    gNdsFighterGCDrawAllLoopSetAirCount = 0;
    gNdsFighterGCDrawAllLoopWaitSetStatusCount = 0;
    gNdsFighterGCDrawAllLoopRunBrakeEndCount = 0;
    gNdsFighterGCDrawAllLoopJumpAnimEndCount = 0;
    gNdsFighterGCDrawAllLoopLandingEndCount = 0;
    gNdsFighterGCDrawAllLoopDeferredInterruptCheckCount = 0;
    gNdsFighterGCDrawAllLoopUnexpectedStatusCount = 0;
    gNdsFighterGCDrawAllLoopDeniedStatusCount = 0;
    gNdsFighterGCDrawAllLoopDisplayProbeCount = 0;
    gNdsFighterGCDrawAllLoopGameplayUpdateCount = 0;
    gNdsFighterGCDrawAllLoopDrawCallCount = 0;
    gNdsFighterGCDrawAllLoopMatrixCallCount = 0;
    gNdsFighterGCDrawAllLoopRootYDriftCount = 0;
    gNdsFighterGCDrawAllLoopGADriftCount = 0;
    gNdsFighterMarioFoxStageGCDrawAllLoopResult = 0;
    gNdsFighterMarioFoxStageGCDrawAllLoopSafeResult = 0;
    gNdsFighterMarioFoxStageGCDrawAllLoopMask = 0;
    gNdsFighterMarioFoxStageGCDrawAllLoopDeferredMask = 0;
    gNdsFighterMarioFoxStageGCDrawAllLoopCount = 0;
    gNdsStageGCDrawAllLoopPrepared = 0;
    gNdsStageGCDrawAllLoopBaseResultSeen = 0;
    gNdsStageGCDrawAllLoopDrawAllCount = 0;
    gNdsStageGCDrawAllLoopCameraCallbackCount = 0;
    gNdsStageGCDrawAllLoopCapturedDisplayCount = 0;
    gNdsStageGCDrawAllLoopLayerCaptureMask = 0;
    gNdsStageGCDrawAllLoopMapCaptureMask = 0;
    gNdsStageGCDrawAllLoopDObjDrawCallbackCount = 0;
    gNdsStageGCDrawAllLoopDObjDrawKindMask = 0;
    gNdsStageGCDrawAllLoopLayerDObjMask = 0;
    gNdsStageGCDrawAllLoopMapDObjMask = 0;
    gNdsStageGCDrawAllLoopLayerDLReadyMask = 0;
    gNdsStageGCDrawAllLoopMapDLReadyMask = 0;
    gNdsStageGCDrawAllLoopLayerMObjMask = 0;
    gNdsStageGCDrawAllLoopMapMObjMask = 0;
    gNdsStageGCDrawAllLoopNonStageCaptureCount = 0;
    gNdsStageGCDrawAllLoopFighterDisplayCallbackCount = 0;
    gNdsStageGCDrawAllLoopUnexpectedSceneCount = 0;
    gNdsStageGCDrawAllLoopManualDisplayCallCount = 0;
    gNdsStageGCDrawAllLoopGObjCountBefore = 0;
    gNdsStageGCDrawAllLoopGObjCountAfter = 0;
    gNdsStageGCDrawAllLoopGObjCountDelta = 0;
    gNdsStageGCDrawAllLoopPreviewCommitDelta = 0;
    gNdsStageGCDrawAllLoopTotalPixelCount = 0;
    gNdsStageGCDrawAllLoopCompatMask = 0;
    gNdsStageGCDrawAllLoopHardwareSubmitCount = 0;
    gNdsStageGCDrawAllLoopHardwareTriangleCount = 0;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount = 0;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount = 0;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount = 0;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount = 0;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount = 0;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount = 0;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount = 0;
    gNdsStageGCDrawAllLoopHardwareTextureFormatMask = 0;
    gNdsStageGCDrawAllLoopHardwareTextureMaxWidth = 0;
    gNdsStageGCDrawAllLoopHardwareTextureMaxHeight = 0;
    gNdsStageGCDrawAllLoopHardwareFighterSubmitCount = 0;
    gNdsStageGCDrawAllLoopHardwareFighterTriangleCount = 0;
    gNdsFighterDisplayContractSelectedCount = 0;
    gNdsFighterDisplayContractHiddenCount = 0;
    gNdsFighterDisplayContractNoTextureCount = 0;
    gNdsFighterDisplayContractSubmittedCount = 0;
    gNdsFighterDisplayContractGeometryMode = 0;
    gNdsFighterDisplayContractCycleType = 0;
    gNdsFighterDisplayContractRenderMode = 0;
    gNdsFighterDisplayContractLightCount = 0;
    gNdsFighterDisplayContractLightDirectionCount = 0;
    gNdsFighterDisplayContractBoundsPassCount = 0;
    gNdsFighterDisplayContractBoundsFailCount = 0;
    gNdsFighterDisplayContractBoundsXBits = 0;
    gNdsFighterDisplayContractBoundsYBits = 0;
    gNdsStageGCDrawAllLoopHardwareCarrySeedCount = 0;
    gNdsStageGCDrawAllLoopHardwareCarryCaptureCount = 0;
    gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount = 0;
    gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount = 0;
    gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount = 0;
    gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount = 0;
    gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount = 0;
    gNdsFighterMarioFoxStageCollisionLoopResult = 0;
    gNdsFighterMarioFoxStageCollisionLoopSafeResult = 0;
    gNdsFighterMarioFoxStageCollisionLoopMask = 0;
    gNdsFighterMarioFoxStageCollisionLoopDeferredMask = 0;
    gNdsFighterMarioFoxStageCollisionLoopCount = 0;
    gNdsStageCollisionLoopPrepared = 0;
    gNdsStageCollisionLoopBaseStageDrawSeen = 0;
    gNdsStageCollisionLoopGeometryReady = 0;
    gNdsStageCollisionLoopGroundDataReady = 0;
    gNdsStageCollisionLoopYakumonoCount = 0;
    gNdsStageCollisionLoopMapObjCount = 0;
    gNdsStageCollisionLoopFloorLineCount = 0;
    gNdsStageCollisionLoopTotalLineCount = 0;
    gNdsStageCollisionLoopProjectCallCount = 0;
    gNdsStageCollisionLoopGeometryProjectCallCount = 0;
    gNdsStageCollisionLoopLegacyFlatFallbackCount = 0;
    gNdsStageCollisionLoopNoGeometryCount = 0;
    gNdsStageCollisionLoopOutOfRangeLineCount = 0;
    gNdsStageCollisionLoopBadVertexCount = 0;
    gNdsStageCollisionLoopDivisionGuardCount = 0;
    gNdsStageCollisionLoopProbeCount = 0;
    gNdsStageCollisionLoopProbeHitCount = 0;
    gNdsStageCollisionLoopProbeMissCount = 0;
    gNdsStageCollisionLoopOffstageMissCount = 0;
    gNdsStageCollisionLoopBelowFloorMissCount = 0;
    gNdsStageCollisionLoopP0ProjectCount = 0;
    gNdsStageCollisionLoopP1ProjectCount = 0;
    gNdsStageCollisionLoopP0HitCount = 0;
    gNdsStageCollisionLoopP1HitCount = 0;
    gNdsStageCollisionLoopP0FloorLineID = -1;
    gNdsStageCollisionLoopP1FloorLineID = -1;
    gNdsStageCollisionLoopP0FloorKind = 0xffffffffu;
    gNdsStageCollisionLoopP1FloorKind = 0xffffffffu;
    gNdsStageCollisionLoopP0FloorLineIsFloor = 0;
    gNdsStageCollisionLoopP1FloorLineIsFloor = 0;
    gNdsStageCollisionLoopFloorGroupID = -1;
    gNdsStageCollisionLoopFloorGroupCount = 0;
    gNdsStageCollisionLoopFloorLineMin = -1;
    gNdsStageCollisionLoopFloorLineMaxExclusive = -1;
    gNdsStageCollisionLoopNonFloorCandidateCount = 0;
    gNdsStageCollisionLoopYakumonoDObjDeferredCount = 0;
    gNdsStageCollisionLoopYakumonoDObjUnsafeIndexGuardCount = 0;
    gNdsStageCollisionLoopP0FloorDistMilli = 0;
    gNdsStageCollisionLoopP1FloorDistMilli = 0;
    gNdsStageCollisionLoopP0FloorFlags = 0;
    gNdsStageCollisionLoopP1FloorFlags = 0;
    gNdsStageCollisionLoopP0FloorAngleX1000 = 0;
    gNdsStageCollisionLoopP0FloorAngleY1000 = 0;
    gNdsStageCollisionLoopP1FloorAngleX1000 = 0;
    gNdsStageCollisionLoopP1FloorAngleY1000 = 0;
    gNdsStageCollisionLoopP0EdgeLX = 0;
    gNdsStageCollisionLoopP0EdgeLY = 0;
    gNdsStageCollisionLoopP0EdgeRX = 0;
    gNdsStageCollisionLoopP0EdgeRY = 0;
    gNdsStageCollisionLoopP1EdgeLX = 0;
    gNdsStageCollisionLoopP1EdgeLY = 0;
    gNdsStageCollisionLoopP1EdgeRX = 0;
    gNdsStageCollisionLoopP1EdgeRY = 0;
    gNdsStageCollisionLoopP0RootYFinalMilli = 0;
    gNdsStageCollisionLoopP1RootYFinalMilli = 0;
    gNdsStageCollisionLoopP0FloorYMilli = 0;
    gNdsStageCollisionLoopP1FloorYMilli = 0;
    gNdsStageCollisionLoopP0FloorOK = 0;
    gNdsStageCollisionLoopP1FloorOK = 0;
    gNdsStageCollisionLoopGObjDelta = 0;
    gNdsStageCollisionLoopUnexpectedSceneCount = 0;
    gNdsStageCollisionLoopUnexpectedStatusCount = 0;
    gNdsStageCollisionLoopUnsafeFallbackAfterPrepareCount = 0;
    gNdsFighterMarioFoxStageFloorFollowLoopResult = 0;
    gNdsFighterMarioFoxStageFloorFollowLoopSafeResult = 0;
    gNdsFighterMarioFoxStageFloorFollowLoopMask = 0;
    gNdsFighterMarioFoxStageFloorFollowLoopDeferredMask = 0;
    gNdsFighterMarioFoxStageFloorFollowLoopCount = 0;
    gNdsStageFloorFollowLoopPrepared = 0;
    gNdsStageFloorFollowLoopBaseDrawSeen = 0;
    gNdsStageFloorFollowLoopBaseCollisionSeen = 0;
    gNdsStageFloorFollowLoopGeometryReady = 0;
    gNdsStageFloorFollowLoopInitialSeedCount = 0;
    gNdsStageFloorFollowLoopInitialAdoptCount = 0;
    gNdsStageFloorFollowLoopFinalRecenterCount = 0;
    gNdsStageFloorFollowLoopFinalAdoptCount = 0;
    gNdsStageFloorFollowLoopMapUpdateCount = 0;
    gNdsStageFloorFollowLoopP0MapUpdateCount = 0;
    gNdsStageFloorFollowLoopP1MapUpdateCount = 0;
    gNdsStageFloorFollowLoopProjectCallCount = 0;
    gNdsStageFloorFollowLoopGeometryHitCount = 0;
    gNdsStageFloorFollowLoopGeometryMissCount = 0;
    gNdsStageFloorFollowLoopNoGeometryCount = 0;
    gNdsStageFloorFollowLoopNonFloorLineCount = 0;
    gNdsStageFloorFollowLoopClampCount = 0;
    gNdsStageFloorFollowLoopNoClampCount = 0;
    gNdsStageFloorFollowLoopP0HitCount = 0;
    gNdsStageFloorFollowLoopP1HitCount = 0;
    gNdsStageFloorFollowLoopP0FloorLineID = -1;
    gNdsStageFloorFollowLoopP1FloorLineID = -1;
    gNdsStageFloorFollowLoopP0FloorKind = 0xffffffffu;
    gNdsStageFloorFollowLoopP1FloorKind = 0xffffffffu;
    gNdsStageFloorFollowLoopP0FloorLineIsFloor = 0;
    gNdsStageFloorFollowLoopP1FloorLineIsFloor = 0;
    gNdsStageFloorFollowLoopP0InitialRootXMilli = 0;
    gNdsStageFloorFollowLoopP1InitialRootXMilli = 0;
    gNdsStageFloorFollowLoopP0FinalRootXMilli = 0;
    gNdsStageFloorFollowLoopP1FinalRootXMilli = 0;
    gNdsStageFloorFollowLoopP0RootXDeltaMilli = 0;
    gNdsStageFloorFollowLoopP1RootXDeltaMilli = 0;
    gNdsStageFloorFollowLoopP0FinalRootYMilli = 0;
    gNdsStageFloorFollowLoopP1FinalRootYMilli = 0;
    gNdsStageFloorFollowLoopP0FloorYMilli = 0;
    gNdsStageFloorFollowLoopP1FloorYMilli = 0;
    gNdsStageFloorFollowLoopP0FinalDriftMilli = 0;
    gNdsStageFloorFollowLoopP1FinalDriftMilli = 0;
    gNdsStageFloorFollowLoopP0MaxDriftMilli = 0;
    gNdsStageFloorFollowLoopP1MaxDriftMilli = 0;
    gNdsStageFloorFollowLoopMaxDriftMilli = 0;
    gNdsStageFloorFollowLoopP0FloorOK = 0;
    gNdsStageFloorFollowLoopP1FloorOK = 0;
    gNdsStageFloorFollowLoopP0FloorVisitMask = 0;
    gNdsStageFloorFollowLoopP1FloorVisitMask = 0;
    gNdsStageFloorFollowLoopP0StatusFinal = 0xffffffffu;
    gNdsStageFloorFollowLoopP1StatusFinal = 0xffffffffu;
    gNdsStageFloorFollowLoopP0GAFinal = 0xffffffffu;
    gNdsStageFloorFollowLoopP1GAFinal = 0xffffffffu;
    gNdsFighterMarioFoxStageFloorEdgeLoopResult = 0;
    gNdsFighterMarioFoxStageFloorEdgeLoopSafeResult = 0;
    gNdsFighterMarioFoxStageFloorEdgeLoopMask = 0;
    gNdsFighterMarioFoxStageFloorEdgeLoopDeferredMask = 0;
    gNdsFighterMarioFoxStageFloorEdgeLoopCount = 0;
    gNdsStageFloorEdgeLoopPrepared = 0;
    gNdsStageFloorEdgeLoopGeometryReady = 0;
    gNdsStageFloorEdgeLoopSelectedLineID = -1;
    gNdsStageFloorEdgeLoopSelectedLineKind = 0xffffffffu;
    gNdsStageFloorEdgeLoopSelectedVertexCount = 0;
    gNdsStageFloorEdgeLoopLeftXMilli = 0;
    gNdsStageFloorEdgeLoopRightXMilli = 0;
    gNdsStageFloorEdgeLoopWidthMilli = 0;
    gNdsStageFloorEdgeLoopP0StartDistMilli = 0;
    gNdsStageFloorEdgeLoopP1StartDistMilli = 0;
    gNdsStageFloorEdgeLoopP0FinalDistMilli = 0;
    gNdsStageFloorEdgeLoopP1FinalDistMilli = 0;
    gNdsStageFloorEdgeLoopP0DeltaDistMilli = 0;
    gNdsStageFloorEdgeLoopP1DeltaDistMilli = 0;
    gNdsStageFloorEdgeLoopP0MinDistMilli = 0x7fffffff;
    gNdsStageFloorEdgeLoopP1MinDistMilli = 0x7fffffff;
    gNdsStageFloorEdgeLoopP0ApproachOK = 0;
    gNdsStageFloorEdgeLoopP1ApproachOK = 0;
    gNdsStageFloorEdgeLoopP0NearEdgeOK = 0;
    gNdsStageFloorEdgeLoopP1NearEdgeOK = 0;
    gNdsStageFloorEdgeLoopP0FloorOK = 0;
    gNdsStageFloorEdgeLoopP1FloorOK = 0;
    gNdsStageFloorEdgeLoopP0FloorVisitMask = 0;
    gNdsStageFloorEdgeLoopP1FloorVisitMask = 0;
    gNdsStageFloorEdgeLoopInsideProbeCount = 0;
    gNdsStageFloorEdgeLoopInsideProbeHitCount = 0;
    gNdsStageFloorEdgeLoopOutsideProbeCount = 0;
    gNdsStageFloorEdgeLoopOutsideProbeMissCount = 0;
    gNdsStageFloorEdgeLoopOutsideProbeUnexpectedHitCount = 0;
    gNdsStageFloorEdgeLoopFCCommonCallCount = 0;
    gNdsStageFloorEdgeLoopFCCommonHitCount = 0;
    gNdsStageFloorEdgeLoopLineTypeCallCount = 0;
    gNdsStageFloorEdgeLoopVertexPositionCallCount = 0;
    gNdsStageFloorEdgeLoopEdgeUnderLCallCount = 0;
    gNdsStageFloorEdgeLoopEdgeUnderRCallCount = 0;
    gNdsStageFloorEdgeLoopEdgeUnderDeferredCount = 0;
    gNdsStageFloorEdgeLoopMapUpdateCount = 0;
    gNdsStageFloorEdgeLoopP0MapUpdateCount = 0;
    gNdsStageFloorEdgeLoopP1MapUpdateCount = 0;
    gNdsStageFloorEdgeLoopPreClampDriftSampleCount = 0;
    gNdsStageFloorEdgeLoopPreClampCount = 0;
    gNdsStageFloorEdgeLoopP0MaxPreClampDriftMilli = 0;
    gNdsStageFloorEdgeLoopP1MaxPreClampDriftMilli = 0;
    gNdsStageFloorEdgeLoopMaxPreClampDriftMilli = 0;
    gNdsStageFloorEdgeLoopFinalRecenterCount = 0;
    gNdsStageFloorEdgeLoopFinalAdoptCount = 0;
    gNdsStageFloorEdgeLoopUnexpectedSceneCount = 0;
    gNdsStageFloorEdgeLoopUnexpectedStatusCount = 0;
    gNdsStageFloorEdgeLoopUnsafeFallbackAfterPrepareCount = 0;
    gNdsFighterMarioFoxStageMPProcessFloorLoopResult = 0;
    gNdsFighterMarioFoxStageMPProcessFloorLoopSafeResult = 0;
    gNdsFighterMarioFoxStageMPProcessFloorLoopMask = 0;
    gNdsFighterMarioFoxStageMPProcessFloorLoopDeferredMask = 0;
    gNdsFighterMarioFoxStageMPProcessFloorLoopCount = 0;
    gNdsStageMPProcessFloorLoopPrepared = 0;
    gNdsStageMPProcessFloorLoopBaseFloorEdgeSeen = 0;
    gNdsStageMPProcessFloorLoopAdapterBuildCount = 0;
    gNdsStageMPProcessFloorLoopAdapterCopyBackCount = 0;
    gNdsStageMPProcessFloorLoopAdapterFallbackLRCount = 0;
    gNdsStageMPProcessFloorLoopProjectFloorIDCallCount = 0;
    gNdsStageMPProcessFloorLoopProjectFloorIDHitCount = 0;
    gNdsStageMPProcessFloorLoopProjectFloorIDMissCount = 0;
    gNdsStageMPProcessFloorLoopTestNewCallCount = 0;
    gNdsStageMPProcessFloorLoopTestNewHitCount = 0;
    gNdsStageMPProcessFloorLoopTestNewMissCount = 0;
    gNdsStageMPProcessFloorLoopTestNewEdgeBranchCount = 0;
    gNdsStageMPProcessFloorLoopTestNewSetProjectCount = 0;
    gNdsStageMPProcessFloorLoopSetLandingFloorCallCount = 0;
    gNdsStageMPProcessFloorLoopSetCollideFloorCallCount = 0;
    gNdsStageMPProcessFloorLoopFCCommonPositiveDistCount = 0;
    gNdsStageMPProcessFloorLoopFCCommonNegativeDistCount = 0;
    gNdsStageMPProcessFloorLoopFCCommonZeroDistCount = 0;
    gNdsStageMPProcessFloorLoopP0UpdateCount = 0;
    gNdsStageMPProcessFloorLoopP1UpdateCount = 0;
    gNdsStageMPProcessFloorLoopP0HitCount = 0;
    gNdsStageMPProcessFloorLoopP1HitCount = 0;
    gNdsStageMPProcessFloorLoopP0MissCount = 0;
    gNdsStageMPProcessFloorLoopP1MissCount = 0;
    gNdsStageMPProcessFloorLoopP0FinalLineID = -1;
    gNdsStageMPProcessFloorLoopP1FinalLineID = -1;
    gNdsStageMPProcessFloorLoopP0FinalLineIsFloor = 0;
    gNdsStageMPProcessFloorLoopP1FinalLineIsFloor = 0;
    gNdsStageMPProcessFloorLoopP0FinalMaskStat = 0;
    gNdsStageMPProcessFloorLoopP1FinalMaskStat = 0;
    gNdsStageMPProcessFloorLoopP0FinalDistMilli = 0;
    gNdsStageMPProcessFloorLoopP1FinalDistMilli = 0;
    gNdsStageMPProcessFloorLoopP0RootYMilli = 0;
    gNdsStageMPProcessFloorLoopP1RootYMilli = 0;
    gNdsStageMPProcessFloorLoopInsideProbeCount = 0;
    gNdsStageMPProcessFloorLoopInsideProbeHitCount = 0;
    gNdsStageMPProcessFloorLoopOutsideProbeCount = 0;
    gNdsStageMPProcessFloorLoopOutsideProbeMissCount = 0;
    gNdsStageMPProcessFloorLoopBelowFloorProbeCount = 0;
    gNdsStageMPProcessFloorLoopBelowFloorPositiveDistCount = 0;
    gNdsStageMPProcessFloorLoopNoFinalRecenterCount = 0;
    gNdsStageMPProcessFloorLoopUnexpectedSceneCount = 0;
    gNdsStageMPProcessFloorLoopUnexpectedStatusCount = 0;
    gNdsStageMPProcessFloorLoopUnsafeCount = 0;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopResult = 0;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopSafeResult = 0;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopMask = 0;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopDeferredMask = 0;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopCount = 0;
    gNdsStageMPUpdateFloorLoopPrepared = 0;
    gNdsStageMPUpdateFloorLoopBaseMPProcessSeen = 0;
    gNdsStageMPUpdateFloorLoopAdapterBuildCount = 0;
    gNdsStageMPUpdateFloorLoopAdapterCopyBackCount = 0;
    gNdsStageMPUpdateFloorLoopAdapterFallbackLRCount = 0;
    gNdsStageMPUpdateFloorLoopUpdateMainCallCount = 0;
    gNdsStageMPUpdateFloorLoopUpdateMainReturnTrueCount = 0;
    gNdsStageMPUpdateFloorLoopUpdateMainReturnFalseCount = 0;
    gNdsStageMPUpdateFloorLoopUpdateMainStepCount = 0;
    gNdsStageMPUpdateFloorLoopUpdateMainMaxStepCount = 0;
    gNdsStageMPUpdateFloorLoopUpdateMainSplitCount = 0;
    gNdsStageMPUpdateFloorLoopUpdateMainCapCount = 0;
    gNdsStageMPUpdateFloorLoopTranslateResetCount = 0;
    gNdsStageMPUpdateFloorLoopProcCollCallCount = 0;
    gNdsStageMPUpdateFloorLoopAllCollisionsCallCount = 0;
    gNdsStageMPUpdateFloorLoopAllCollisionsFloorHitCount = 0;
    gNdsStageMPUpdateFloorLoopAllCollisionsFloorMissCount = 0;
    gNdsStageMPUpdateFloorLoopAllCollisionsCliffEdgeBranchCount = 0;
    gNdsStageMPUpdateFloorLoopAllCollisionsStopEdgeBranchCount = 0;
    gNdsStageMPUpdateFloorLoopAllCollisionsDefaultEndCount = 0;
    gNdsStageMPUpdateFloorLoopAllCollisionsWallDeferredCount = 0;
    gNdsStageMPUpdateFloorLoopAllCollisionsCeilDeferredCount = 0;
    gNdsStageMPUpdateFloorLoopAllCollisionsFloorEdgeAdjustDeferredCount = 0;
    gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount = 0;
    gNdsStageMPUpdateFloorLoopCheckFloorCallCount = 0;
    gNdsStageMPUpdateFloorLoopCheckCliffEdgeCallCount = 0;
    gNdsStageMPUpdateFloorLoopCheckFloorHitCount = 0;
    gNdsStageMPUpdateFloorLoopCheckCliffEdgeHitCount = 0;
    gNdsStageMPUpdateFloorLoopCheckFloorMissCount = 0;
    gNdsStageMPUpdateFloorLoopCheckCliffEdgeMissCount = 0;
    gNdsStageMPUpdateFloorLoopInsideProbeCount = 0;
    gNdsStageMPUpdateFloorLoopInsideProbeHitCount = 0;
    gNdsStageMPUpdateFloorLoopOutsideProbeCount = 0;
    gNdsStageMPUpdateFloorLoopOutsideProbeMissCount = 0;
    gNdsStageMPUpdateFloorLoopBelowFloorProbeCount = 0;
    gNdsStageMPUpdateFloorLoopBelowFloorHitCount = 0;
    gNdsStageMPUpdateFloorLoopSplitProbeCount = 0;
    gNdsStageMPUpdateFloorLoopSplitProbeStepCount = 0;
    gNdsStageMPUpdateFloorLoopP0UpdateCount = 0;
    gNdsStageMPUpdateFloorLoopP1UpdateCount = 0;
    gNdsStageMPUpdateFloorLoopP0HitCount = 0;
    gNdsStageMPUpdateFloorLoopP1HitCount = 0;
    gNdsStageMPUpdateFloorLoopP0MissCount = 0;
    gNdsStageMPUpdateFloorLoopP1MissCount = 0;
    gNdsStageMPUpdateFloorLoopP0PosDiffXMilli = 0;
    gNdsStageMPUpdateFloorLoopP1PosDiffXMilli = 0;
    gNdsStageMPUpdateFloorLoopP0PosDiffYMilli = 0;
    gNdsStageMPUpdateFloorLoopP1PosDiffYMilli = 0;
    gNdsStageMPUpdateFloorLoopP0RootXBeforeMilli = 0;
    gNdsStageMPUpdateFloorLoopP1RootXBeforeMilli = 0;
    gNdsStageMPUpdateFloorLoopP0RootXFinalMilli = 0;
    gNdsStageMPUpdateFloorLoopP1RootXFinalMilli = 0;
    gNdsStageMPUpdateFloorLoopP0RootYFinalMilli = 0;
    gNdsStageMPUpdateFloorLoopP1RootYFinalMilli = 0;
    gNdsStageMPUpdateFloorLoopP0FinalLineID = -1;
    gNdsStageMPUpdateFloorLoopP1FinalLineID = -1;
    gNdsStageMPUpdateFloorLoopP0FinalLineIsFloor = 0;
    gNdsStageMPUpdateFloorLoopP1FinalLineIsFloor = 0;
    gNdsStageMPUpdateFloorLoopP0FinalMaskStat = 0;
    gNdsStageMPUpdateFloorLoopP1FinalMaskStat = 0;
    gNdsStageMPUpdateFloorLoopP0FloorOK = 0;
    gNdsStageMPUpdateFloorLoopP1FloorOK = 0;
    gNdsStageMPUpdateFloorLoopNoFinalRecenterCount = 0;
    gNdsStageMPUpdateFloorLoopFallDeniedCount = 0;
    gNdsStageMPUpdateFloorLoopOttottoDeniedCount = 0;
    gNdsStageMPUpdateFloorLoopUnexpectedSceneCount = 0;
    gNdsStageMPUpdateFloorLoopUnexpectedStatusCount = 0;
    gNdsStageMPUpdateFloorLoopUnsafeCount = 0;
    gNdsFighterMarioFoxStageMPSweepFloorLoopResult = 0;
    gNdsFighterMarioFoxStageMPSweepFloorLoopSafeResult = 0;
    gNdsFighterMarioFoxStageMPSweepFloorLoopMask = 0;
    gNdsFighterMarioFoxStageMPSweepFloorLoopDeferredMask = 0;
    gNdsFighterMarioFoxStageMPSweepFloorLoopCount = 0;
    gNdsStageMPSweepFloorLoopPrepared = 0;
    gNdsStageMPSweepFloorLoopBaseMPUpdateSeen = 0;
    gNdsStageMPSweepFloorLoopCheckFloorCallCount = 0;
    gNdsStageMPSweepFloorLoopCheckFloorHitCount = 0;
    gNdsStageMPSweepFloorLoopCheckFloorMissCount = 0;
    gNdsStageMPSweepFloorLoopLineSweepSameCallCount = 0;
    gNdsStageMPSweepFloorLoopLineSweepSameHitCount = 0;
    gNdsStageMPSweepFloorLoopLineSweepSameMissCount = 0;
    gNdsStageMPSweepFloorLoopLineSweepDiffCallCount = 0;
    gNdsStageMPSweepFloorLoopLineSweepDiffHitCount = 0;
    gNdsStageMPSweepFloorLoopLineSweepDiffMissCount = 0;
    gNdsStageMPSweepFloorLoopLineSweepVisitCount = 0;
    gNdsStageMPSweepFloorLoopLineSweepCandidateCount = 0;
    gNdsStageMPSweepFloorLoopLineSweepRejectSameLineCount = 0;
    gNdsStageMPSweepFloorLoopLineSweepAcceptNewLineCount = 0;
    gNdsStageMPSweepFloorLoopSecondFloorCallCount = 0;
    gNdsStageMPSweepFloorLoopSecondFloorHitCount = 0;
    gNdsStageMPSweepFloorLoopSecondFloorMissCount = 0;
    gNdsStageMPSweepFloorLoopLandingFloorCallCount = 0;
    gNdsStageMPSweepFloorLoopFloorEdgeAdjustCallCount = 0;
    gNdsStageMPSweepFloorLoopFloorEdgeAdjustDeferredCount = 0;
    gNdsStageMPSweepFloorLoopMaskCurrFloorCount = 0;
    gNdsStageMPSweepFloorLoopMaskStatFloorEdgeClearCount = 0;
    gNdsStageMPSweepFloorLoopIsCollEndClearCount = 0;
    gNdsStageMPSweepFloorLoopSameLineProbeCount = 0;
    gNdsStageMPSweepFloorLoopSameLineProbeHitCount = 0;
    gNdsStageMPSweepFloorLoopDiffLineProbeCount = 0;
    gNdsStageMPSweepFloorLoopDiffLineProbeHitCount = 0;
    gNdsStageMPSweepFloorLoopNoHitProbeCount = 0;
    gNdsStageMPSweepFloorLoopNoHitProbeMissCount = 0;
    gNdsStageMPSweepFloorLoopProbeLineID = -1;
    gNdsStageMPSweepFloorLoopAltLineID = -1;
    gNdsStageMPSweepFloorLoopP0FinalLineID = -1;
    gNdsStageMPSweepFloorLoopP1FinalLineID = -1;
    gNdsStageMPSweepFloorLoopP0FinalLineIsFloor = 0;
    gNdsStageMPSweepFloorLoopP1FinalLineIsFloor = 0;
    gNdsStageMPSweepFloorLoopP0FloorOK = 0;
    gNdsStageMPSweepFloorLoopP1FloorOK = 0;
    gNdsStageMPSweepFloorLoopUnsafeCount = 0;
    gNdsSCVSBattleStageResult = 0;
    gNdsSCVSBattleStageMask = 0;
    gNdsSCVSBattleStageGKind = 0;
    gNdsSCVSBattleStageGroundDataReady = 0;
    gNdsSCVSBattleStageGeometryReady = 0;
    gNdsSCVSBattleStageMapNodesReady = 0;
    gNdsSCVSBattleStageBGM = 0;
    gNdsSCVSBattleStageLightAngleXBits = 0;
    gNdsSCVSBattleStageLightAngleYBits = 0;
    gNdsSCVSBattleStageDeferredMask = 0;
    gNdsPupupuGroundSetupResult = 0;
    gNdsPupupuGroundDisplayResult = 0;
    gNdsPupupuGroundGObjResult = 0;
    gNdsPupupuGroundSetupMask = 0;
    gNdsPupupuGroundDeferredMask = 0;
    gNdsPupupuGroundLayerGObjCount = 0;
    gNdsPupupuGroundLayerGObjMask = 0;
    gNdsPupupuGroundLayerDObjMask = 0;
    gNdsPupupuGroundLayerMObjMask = 0;
    gNdsPupupuGroundLayerAnimMask = 0;
    gNdsPupupuGroundMapGObjCount = 0;
    gNdsPupupuGroundMapGObjMask = 0;
    gNdsPupupuGroundMapHeadReady = 0;
    gNdsPupupuGroundMapHeadOffset = 0;
    gNdsPupupuGroundRootGObjID = 0;
    gNdsPupupuGroundWhispyEyesGObjID = 0;
    gNdsPupupuGroundWhispyMouthGObjID = 0;
    gNdsPupupuGroundFlowersBackGObjID = 0;
    gNdsPupupuGroundFlowersFrontGObjID = 0;
    gNdsPupupuGroundParticleBankID = 0;
    gNdsPupupuGroundProcessAttachCount = 0;
    gNdsPupupuGroundNonPupupuStubCallCount = 0;
    gNdsPupupuGroundGObjCountBefore = 0;
    gNdsPupupuGroundGObjCountAfter = 0;
    gNdsPupupuGroundDObjCountAfter = 0;
    gNdsPupupuGroundMObjCountAfter = 0;
    gNdsPupupuGroundAObjCountAfter = 0;
    gNdsPupupuUpdateResult = 0;
    gNdsPupupuUpdateMask = 0;
    gNdsPupupuUpdateTickCount = 0;
    gNdsPupupuUpdateGameStatusBefore = 0;
    gNdsPupupuUpdateGameStatusAfter = 0;
    gNdsPupupuUpdateWhispyStatusBefore = 0;
    gNdsPupupuUpdateWhispyStatusAfterFirst = 0;
    gNdsPupupuUpdateWhispyStatusAfterFinal = 0;
    gNdsPupupuUpdateWindWaitBefore = 0;
    gNdsPupupuUpdateWindWaitAfterFirst = 0;
    gNdsPupupuUpdateWindWaitAfterFinal = 0;
    gNdsPupupuUpdateBlinkWaitBefore = 0;
    gNdsPupupuUpdateBlinkWaitAfterFinal = 0;
    gNdsPupupuUpdateFlowersBackStatusBefore = 0;
    gNdsPupupuUpdateFlowersBackStatusAfterFinal = 0;
    gNdsPupupuUpdateFlowersFrontStatusBefore = 0;
    gNdsPupupuUpdateFlowersFrontStatusAfterFinal = 0;
    gNdsPupupuUpdateMapGObjMaskBefore = 0;
    gNdsPupupuUpdateMapGObjMaskAfter = 0;
    gNdsPupupuUpdateGObjCountBefore = 0;
    gNdsPupupuUpdateGObjCountAfter = 0;
    gNdsPupupuUpdateVelPushCount = 0;
    gNdsPupupuUpdateQuakeCount = 0;
    gNdsPupupuUpdateParticleScriptCount = 0;
    gNdsPupupuUpdateWindFGMCount = 0;
    gNdsOpeningRoomGObjCount = 0;
    gNdsOpeningRoomCameraCount = 0;
    gNdsOpeningRoomDL0Size = 0;
    gNdsOpeningRoomDL1Size = 0;
    gNdsOpeningRoomGraphicsHeapSize = 0;
    gNdsOpeningRoomRdpBufferSize = 0;
    gNdsOpeningRoomMallocCount = 0;
    gNdsOpeningRoomPreAssetResult = 0;
    gNdsOpeningRoomFirstEventResult = 0;
    gNdsOpeningRoomFirstEventTick = 0;
    gNdsOpeningRoomFirstEventProbeMask = 0;
    gNdsOpeningRoomFirstEventPencilsDObjOffset = 0;
    gNdsOpeningRoomFirstEventPencilsAnimOffset = 0;
    gNdsOpeningRoomFirstEventDataResult = 0;
    gNdsOpeningRoomFirstEventDataMask = 0;
    gNdsOpeningRoomFirstEventPencilsDObjEntries = 0;
    gNdsOpeningRoomFirstEventPencilsDLPtrs = 0;
    gNdsOpeningRoomFirstEventPencilsAnimJoints = 0;
    gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode = 0;
    gNdsOpeningRoomFirstEventRunResult = 0;
    gNdsOpeningRoomFirstEventDeferredMask = 0;
    gNdsOpeningRoomFighterDeferredResult = 0;
    gNdsOpeningRoomFighterDeferredKind = 0xffffffffu;
    gNdsOpeningRoomTick380DeferredResult = 0;
    gNdsOpeningRoomTick380DeferredMask = 0;
    gNdsOpeningRoomTick450RunResult = 0;
    gNdsOpeningRoomTick450DeferredMask = 0;
    gNdsOpeningRoomTick500RunResult = 0;
    gNdsOpeningRoomTick500DeferredMask = 0;
    gNdsOpeningRoomTick560RunResult = 0;
    gNdsOpeningRoomTick560DeferredMask = 0;
    gNdsOpeningRoomDrawResult = 0;
    gNdsOpeningRoomDrawBlocker = NDS_OPENING_ROOM_DRAW_BLOCKER_NONE;
    gNdsOpeningRoomDrawTickCount = 0;
    gNdsOpeningRoomDrawFrameCount = 0;
    gNdsOpeningRoomDrawProbeCount = 0;
    gNdsOpeningRoomDrawReuseCount = 0;
    gNdsOpeningRoomDrawCameraCallbackCount = 0;
    gNdsOpeningRoomDrawDisplayCallbackCount = 0;
    gNdsOpeningRoomDrawDObjCallbackCount = 0;
    gNdsOpeningRoomDrawFirstCameraMaskLow = 0;
    gNdsOpeningRoomDrawFirstCameraPriority = 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraFlags = 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraXObjCount = 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraXObjKind0 = 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraXObjKind1 = 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraViewportScaleX = 0;
    gNdsOpeningRoomDrawFirstCameraViewportScaleY = 0;
    gNdsOpeningRoomDrawFirstCameraViewportTransX = 0;
    gNdsOpeningRoomDrawFirstCameraViewportTransY = 0;
    gNdsRdpDefaultViewportSetCount = 0;
    gNdsRdpDefaultViewportScaleX = 0;
    gNdsRdpDefaultViewportScaleY = 0;
    gNdsRdpDefaultViewportTransX = 0;
    gNdsRdpDefaultViewportTransY = 0;
    gNdsRdpDefaultViewportScaleZ = 0;
    gNdsRdpDefaultViewportTransZ = 0;
    gNdsOpeningRoomDrawFirstCameraNear100 = 0;
    gNdsOpeningRoomDrawFirstCameraFar100 = 0;
    gNdsOpeningRoomDrawFirstCameraFovY100 = 0;
    gNdsOpeningRoomDrawFirstCameraEyeX100 = 0;
    gNdsOpeningRoomDrawFirstCameraEyeY100 = 0;
    gNdsOpeningRoomDrawFirstCameraEyeZ100 = 0;
    gNdsOpeningRoomDrawFirstCameraAtX100 = 0;
    gNdsOpeningRoomDrawFirstCameraAtY100 = 0;
    gNdsOpeningRoomDrawFirstCameraAtZ100 = 0;
    gNdsOpeningRoomDrawFirstObjectDLLink = 0xffffffffu;
    gNdsOpeningRoomDrawFirstObjectID = 0xffffffffu;
    gNdsOpeningRoomDrawFirstObjectKind = 0xffffffffu;
    gNdsOpeningRoomDrawFirstCallback = 0;
    gNdsOpeningRoomDrawFirstDObjDL = 0;
    gNdsOpeningRoomDrawFirstDObjMeta = 0;
    gNdsOpeningRoomDrawMaterialCandidateResult = 0;
    gNdsOpeningRoomDrawMaterialCandidateCount = 0;
    gNdsOpeningRoomDrawMaterialCandidateCameraMaskLow = 0;
    gNdsOpeningRoomDrawMaterialCandidateCameraPriority = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateObjectDLLink = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateObjectID = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateObjectKind = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateCallback = 0;
    gNdsOpeningRoomDrawMaterialCandidateDObjDL = 0;
    gNdsOpeningRoomDrawMaterialCandidateDObjMeta = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjCount = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjFlags = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjEffectiveFlags = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjMask = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjTextureCurr = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjTextureNext = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjPaletteIndex = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjLfrac100 = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjFormat = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjSize = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjBlockFormat = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjBlockSize = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjTileWidth = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjTileHeight = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjScrollWidth = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjScrollHeight = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjScaleS100 = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjScaleT100 = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjTranslateS100 = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjTranslateT100 = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjSpriteArray = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjPaletteArray = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjSpriteCurr = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjSpriteNext = 0;
    gNdsOpeningRoomDrawMaterialCandidateMObjPalettePtr = 0;
    gNdsOpeningRoomDrawTextureMaterialResult = 0;
    gNdsOpeningRoomDrawTextureMaterialCandidateCount = 0;
    gNdsOpeningRoomDrawTextureMaterialMObjCount = 0;
    gNdsOpeningRoomDrawTextureMaterialSpriteArrayCount = 0;
    gNdsOpeningRoomDrawTextureMaterialSpriteCurrCount = 0;
    gNdsOpeningRoomDrawTextureMaterialSpriteNextCount = 0;
    gNdsOpeningRoomDrawTextureMaterialObjectDLLink = 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialObjectID = 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialObjectKind = 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialCallback = 0;
    gNdsOpeningRoomDrawTextureMaterialDObjDL = 0;
    gNdsOpeningRoomDrawTextureMaterialDObjMeta = 0;
    gNdsOpeningRoomDrawTextureMaterialMObjFlags = 0;
    gNdsOpeningRoomDrawTextureMaterialMObjEffectiveFlags = 0;
    gNdsOpeningRoomDrawTextureMaterialMObjMask = 0;
    gNdsOpeningRoomDrawTextureMaterialMObjTextureCurr = 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialMObjTextureNext = 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialMObjSpriteArray = 0;
    gNdsOpeningRoomDrawTextureMaterialMObjSpriteCurr = 0;
    gNdsOpeningRoomDrawTextureMaterialMObjSpriteNext = 0;
    gNdsOpeningRoomDrawMaterialBranchResult = 0;
    gNdsOpeningRoomDrawMaterialBranchMObjCount = 0;
    gNdsOpeningRoomDrawMaterialBranchSegmentCommands = 0;
    gNdsOpeningRoomDrawMaterialBranchTableCommands = 0;
    gNdsOpeningRoomDrawMaterialBranchGeneratedCommands = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstMask = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstGeneratedCommands = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleS = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleT = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstTileUls = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstTileUlt = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstTileLrs = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstTileLrt = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstScrollUls = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstScrollUlt = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstScrollLrs = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstScrollLrt = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockTexels = 0;
    gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockDxt = 0;
    gNdsOpeningRoomDrawMaterialEmitResult = 0;
    gNdsOpeningRoomDrawMaterialEmitBlocker =
        NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_NONE;
    gNdsOpeningRoomDrawMaterialEmitUnsupportedMask = 0;
    gNdsOpeningRoomDrawMaterialEmitMObjCount = 0;
    gNdsOpeningRoomDrawMaterialEmitTableCommands = 0;
    gNdsOpeningRoomDrawMaterialEmitGeneratedCommands = 0;
    gNdsOpeningRoomDrawMaterialEmitHeapStart = 0;
    gNdsOpeningRoomDrawMaterialEmitBranchStart = 0;
    gNdsOpeningRoomDrawMaterialEmitHeapAfter = 0;
    gNdsOpeningRoomDrawMaterialEmitHeapBytes = 0;
    gNdsOpeningRoomDrawMaterialEmitFirstTableOp = 0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchOp0 = 0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchOp1 = 0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchOp2 = 0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_0 = 0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_0 = 0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_1 = 0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_1 = 0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_2 = 0;
    gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_2 = 0;
    gNdsOpeningRoomDLPreviewResult = 0;
    gNdsOpeningRoomDLPreviewBlocker =
        NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NONE;
    gNdsOpeningRoomDLPreviewCommandCount = 0;
    gNdsOpeningRoomDLPreviewVertexCount = 0;
    gNdsOpeningRoomDLPreviewTriangleCount = 0;
    gNdsOpeningRoomDLPreviewPixelCount = 0;
    gNdsOpeningRoomDLPreviewFirstOpcode = 0;
    gNdsOpeningRoomDLPreviewUnsupportedOpcode = 0;
    gNdsOpeningRoomDLPreviewUnsupportedCommandCount = 0;
    gNdsOpeningRoomDLPreviewVertexCommandCount = 0;
    gNdsOpeningRoomDLPreviewTriangleCommandCount = 0;
    gNdsOpeningRoomDLPreviewSyncCommandCount = 0;
    gNdsOpeningRoomDLPreviewEndCommandCount = 0;
    gNdsOpeningRoomDLPreviewBranchCommandCount = 0;
    gNdsOpeningRoomDLPreviewBranchCallCount = 0;
    gNdsOpeningRoomDLPreviewBranchJumpCount = 0;
    gNdsOpeningRoomDLPreviewSegmentResolveCount = 0;
    gNdsOpeningRoomDLPreviewColorCommandCount = 0;
    gNdsOpeningRoomDLPreviewPrimColor = 0;
    gNdsOpeningRoomDLPreviewOtherModeCommandCount = 0;
    gNdsOpeningRoomDLPreviewFirstDL = 0;
    gNdsOpeningRoomDLPreviewTransformMask = 0;
    gNdsOpeningRoomDLPreviewXObjCount = 0;
    gNdsOpeningRoomDLPreviewFirstXObjKind = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTranslateX100 = 0;
    gNdsOpeningRoomDLPreviewTranslateY100 = 0;
    gNdsOpeningRoomDLPreviewTranslateZ100 = 0;
    gNdsOpeningRoomDLPreviewRotateX100 = 0;
    gNdsOpeningRoomDLPreviewRotateY100 = 0;
    gNdsOpeningRoomDLPreviewRotateZ100 = 0;
    gNdsOpeningRoomDLPreviewScaleX100 = 0;
    gNdsOpeningRoomDLPreviewScaleY100 = 0;
    gNdsOpeningRoomDLPreviewScaleZ100 = 0;
    gNdsOpeningRoomDLPreviewMinX = 0;
    gNdsOpeningRoomDLPreviewMaxX = 0;
    gNdsOpeningRoomDLPreviewMinY = 0;
    gNdsOpeningRoomDLPreviewMaxY = 0;
    gNdsOpeningRoomDLPreviewProjectionMask = 0;
    gNdsOpeningRoomDLPreviewProjectionMode =
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_MODE_NONE;
    gNdsOpeningRoomDLPreviewProjectionBlocker =
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_NONE;
    gNdsOpeningRoomDLPreviewProjectedVertexCount = 0;
    gNdsOpeningRoomDLPreviewProjectedTriangleCount = 0;
    gNdsOpeningRoomDLPreviewProjectedMinX = 0;
    gNdsOpeningRoomDLPreviewProjectedMaxX = 0;
    gNdsOpeningRoomDLPreviewProjectedMinY = 0;
    gNdsOpeningRoomDLPreviewProjectedMaxY = 0;
    gNdsOpeningRoomDLPreviewProjectedMinDepth100 = 0;
    gNdsOpeningRoomDLPreviewProjectedMaxDepth100 = 0;
    gNdsOpeningRoomDLPreviewFallbackAxis =
        NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_XY;
    gNdsOpeningRoomDLPreviewFallbackArea = 0;
    gNdsOpeningRoomDLPreviewGeometryCommandCount = 0;
    gNdsOpeningRoomDLPreviewGeometryClearMask = 0;
    gNdsOpeningRoomDLPreviewGeometrySetMask = 0;
    gNdsOpeningRoomDLPreviewGeometryFinalMode = 0;
    gNdsOpeningRoomDLPreviewGeometryFlags = 0;
    gNdsOpeningRoomDLPreviewGeometryPositiveWinding = 0;
    gNdsOpeningRoomDLPreviewGeometryNegativeWinding = 0;
    gNdsOpeningRoomDLPreviewGeometryZeroArea = 0;
    gNdsOpeningRoomDLPreviewGeometryDrawnTriangles = 0;
    gNdsOpeningRoomDLPreviewTextureMask = 0;
    gNdsOpeningRoomDLPreviewTextureImage = 0;
    gNdsOpeningRoomDLPreviewTextureFormat = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureSize = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureImageWidth = 0;
    gNdsOpeningRoomDLPreviewTextureTileWidth = 0;
    gNdsOpeningRoomDLPreviewTextureTileHeight = 0;
    gNdsOpeningRoomDLPreviewTextureTexelWidth = 0;
    gNdsOpeningRoomDLPreviewTextureTexelHeight = 0;
    gNdsOpeningRoomDLPreviewTextureLoadTexels = 0;
    gNdsOpeningRoomDLPreviewTextureLoadBlockTile = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureLoadBlockUls = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureLoadBlockUlt = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureLoadBlockLrs = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureLoadBlockDxt = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureTileSizeTile = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureTileSizeUls = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureTileSizeUlt = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureTileSizeLrs = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureTileSizeLrt = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureSamplePixels = 0;
    gNdsOpeningRoomDLPreviewTextureSetTileCount = 0;
    gNdsOpeningRoomDLPreviewTextureCombineW0 = 0;
    gNdsOpeningRoomDLPreviewTextureCombineW1 = 0;
    gNdsOpeningRoomDLPreviewTextureCombineMode =
        NDS_OPENING_ROOM_DL_COMBINE_MODE_UNKNOWN;
    gNdsOpeningRoomDLPreviewTextureCombineFlags = 0;
    gNdsOpeningRoomDLPreviewTextureModulatedPixels = 0;
    gNdsOpeningRoomDLPreviewTextureRenderTile = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureRenderTileLine = 0;
    gNdsOpeningRoomDLPreviewTextureRenderTileTmem = 0;
    gNdsOpeningRoomDLPreviewTextureRenderTilePalette = 0;
    gNdsOpeningRoomDLPreviewTextureRenderTileCms = 0;
    gNdsOpeningRoomDLPreviewTextureRenderTileCmt = 0;
    gNdsOpeningRoomDLPreviewTextureRenderTileMasks = 0;
    gNdsOpeningRoomDLPreviewTextureRenderTileMaskt = 0;
    gNdsOpeningRoomDLPreviewTextureRenderTileShifts = 0;
    gNdsOpeningRoomDLPreviewTextureRenderTileShiftt = 0;
    gNdsOpeningRoomDLPreviewTextureRenderTileFlags = 0;
    gNdsOpeningRoomDLPreviewTextureCommandCount = 0;
    gNdsOpeningRoomDLPreviewTextureReady = 0;
    gNdsOpeningRoomDLPreviewTextureSampleMode =
        NDS_OPENING_ROOM_DL_TEXTURE_SAMPLE_NONE;
    gNdsOpeningRoomDLPreviewTextureScaleS = 0;
    gNdsOpeningRoomDLPreviewTextureScaleT = 0;
    gNdsOpeningRoomDLPreviewTextureLevel = 0;
    gNdsOpeningRoomDLPreviewTextureTile = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureOn = 0;
    gNdsOpeningRoomDLPreviewTextureXParam = 0;
    gNdsOpeningRoomDLPreviewTextureStateFlags = 0;
    gNdsOpeningRoomDLPreviewMaterialCount = 0;
    gNdsOpeningRoomDLPreviewMaterialFlags = 0;
    gNdsOpeningRoomDLPreviewMaterialEffectiveFlags = 0;
    gNdsOpeningRoomDLPreviewMaterialMask = 0;
    gNdsOpeningRoomDLPreviewMaterialTextureCurr = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialTextureNext = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialPaletteIndex = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialLfrac100 = 0;
    gNdsOpeningRoomDLPreviewMaterialFormat = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialSize = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialBlockFormat = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialBlockSize = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialTileWidth = 0;
    gNdsOpeningRoomDLPreviewMaterialTileHeight = 0;
    gNdsOpeningRoomDLPreviewMaterialScrollWidth = 0;
    gNdsOpeningRoomDLPreviewMaterialScrollHeight = 0;
    gNdsOpeningRoomDLPreviewMaterialScaleS100 = 0;
    gNdsOpeningRoomDLPreviewMaterialScaleT100 = 0;
    gNdsOpeningRoomDLPreviewMaterialTranslateS100 = 0;
    gNdsOpeningRoomDLPreviewMaterialTranslateT100 = 0;
    gNdsOpeningRoomDLPreviewMaterialSpriteCurr = 0;
    gNdsOpeningRoomDLPreviewMaterialSpriteNext = 0;
    gNdsOpeningRoomDLPreviewMaterialPalettePtr = 0;
    gNdsOpeningRoomDLPreviewMaterialBranchResult = 0;
    gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
        NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_NONE;
    gNdsOpeningRoomDLPreviewMaterialBranchCommandCount = 0;
    gNdsOpeningRoomDLPreviewMaterialBranchPrimCount = 0;
    gNdsOpeningRoomDLPreviewMaterialBranchEndCount = 0;
    gNdsOpeningRoomDLPreviewMaterialBranchUnsupportedOp = 0;
    gNdsOpeningRoomDLPreviewMaterialBranchFirstOp = 0;
    gNdsOpeningRoomDLPreviewMaterialBranchSecondOp = 0;
    gNdsOpeningRoomDLPreviewMaterialBranchPrimColor = 0;
    gNdsOpeningRoomDLPreviewMaterialBranchPrimLod = 0;
    gNdsOpeningRoomDLPreviewMaterialBranchPrimM = 0;
    gNdsOpeningRoomDLPreviewRendererParsedCommandCount = 0;
    gNdsOpeningRoomDLPreviewRendererStateCommandCount = 0;
    gNdsOpeningRoomDLPreviewRendererSkippedCommandCount = 0;
    gNdsOpeningRoomDLPreviewRendererRenderedCommandCount = 0;
    gNdsOpeningRoomDLPreviewRendererTextureMask = 0;
    gNdsOpeningRoomDLPreviewRendererTextureImage = 0;
    gNdsOpeningRoomDLPreviewRendererTextureFormat = 0xffffffffu;
    gNdsOpeningRoomDLPreviewRendererTextureSize = 0xffffffffu;
    gNdsOpeningRoomDLPreviewRendererTextureImageWidth = 0;
    gNdsOpeningRoomDLPreviewRendererTextureLoadTexels = 0;
    gNdsOpeningRoomDLPreviewRendererTextureSetTileCount = 0;
    gNdsOpeningRoomDLPreviewRendererTextureCommandCount = 0;
    gNdsOpeningRoomDLPreviewRendererTextureStateFlags = 0;
    gNdsOpeningRoomDLPreviewRendererTextureTileWidth = 0;
    gNdsOpeningRoomDLPreviewRendererTextureTileHeight = 0;
    gNdsOpeningRoomDLPreviewRendererTextureRenderTile = 0xffffffffu;
    gNdsOpeningRoomDLPreviewRendererTextureRenderTileLine = 0;
    gNdsOpeningRoomDLPreviewRendererTextureRenderTileFlags = 0;
    gNdsOpeningRoomDLPreviewRendererTextureLoadBlockLrs = 0xffffffffu;
    gNdsOpeningRoomDLPreviewRendererTextureLoadBlockDxt = 0xffffffffu;
    gNdsOpeningRoomMaterialDLProbeResult = 0;
    gNdsOpeningRoomMaterialDLProbeBlocker =
        NDS_OPENING_ROOM_MATERIAL_DL_PROBE_BLOCKER_NONE;
    gNdsOpeningRoomMaterialDLProbeFirstDL = 0;
    gNdsOpeningRoomMaterialDLProbeCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeVertexCount = 0;
    gNdsOpeningRoomMaterialDLProbeTriangleCount = 0;
    gNdsOpeningRoomMaterialDLProbeFirstOpcode = 0;
    gNdsOpeningRoomMaterialDLProbeUnsupportedOpcode = 0;
    gNdsOpeningRoomMaterialDLProbeVertexCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeTriangleCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeSyncCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeEndCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeBranchCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeOtherModeCommandCount = 0;
    gNdsOpeningRoomMaterialDLProbeUnsupportedCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandResult = 0;
    gNdsOpeningRoomMaterialDLExpandBlocker =
        NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_NONE;
    gNdsOpeningRoomMaterialDLExpandFirstDL = 0;
    gNdsOpeningRoomMaterialDLExpandFirstBranchDL = 0;
    gNdsOpeningRoomMaterialDLExpandFirstResolvedBranchDL = 0;
    gNdsOpeningRoomMaterialDLExpandCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandVertexCount = 0;
    gNdsOpeningRoomMaterialDLExpandTriangleCount = 0;
    gNdsOpeningRoomMaterialDLExpandFirstOpcode = 0;
    gNdsOpeningRoomMaterialDLExpandUnsupportedOpcode = 0;
    gNdsOpeningRoomMaterialDLExpandVertexCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandTriangleCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandSyncCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandEndCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandBranchCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandBranchCallCount = 0;
    gNdsOpeningRoomMaterialDLExpandBranchJumpCount = 0;
    gNdsOpeningRoomMaterialDLExpandSegmentResolveCount = 0;
    gNdsOpeningRoomMaterialDLExpandOtherModeCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandColorCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandUnsupportedCommandCount = 0;
    gNdsOpeningRoomMaterialDLExpandMaxDepth = 0;
    sNdsOpeningRoomCurrentDrawCameraGObj = NULL;
    sNdsOpeningRoomPreviewCameraGObj = NULL;
    sNdsOpeningRoomFallbackPreviewCameraGObj = NULL;
    sNdsOpeningRoomFallbackPreviewGObj = NULL;
    sNdsOpeningRoomFallbackPreviewDObj = NULL;
    sNdsOpeningRoomFallbackPreviewDL = NULL;
    sNdsOpeningRoomMaterialPreviewCameraGObj = NULL;
    sNdsOpeningRoomMaterialPreviewGObj = NULL;
    sNdsOpeningRoomMaterialPreviewDObj = NULL;
    sNdsOpeningRoomMaterialPreviewDL = NULL;
    gNdsOpeningRoomOutsideAssetMask = 0;
    gNdsOpeningRoomOutsideDisplayListOffset = 0;
    gNdsOpeningRoomOutsideCreateResult = 0;
    gNdsOpeningRoomOutsideCreateMask = 0;
    gNdsOpeningRoomOutsideCreateGObjCount = 0;
    gNdsOpeningRoomOutsideGObjDelta = 0;
    gNdsOpeningRoomOutsideDObjDelta = 0;
    gNdsOpeningRoomOutsideXObjDelta = 0;
    gNdsOpeningRoomOutsideDisplaySet = 0;
    gNdsOpeningRoomHazeAssetMask = 0;
    gNdsOpeningRoomHazeDisplayListOffset = 0;
    gNdsOpeningRoomHazeCreateResult = 0;
    gNdsOpeningRoomHazeCreateMask = 0;
    gNdsOpeningRoomHazeCreateGObjCount = 0;
    gNdsOpeningRoomHazeGObjDelta = 0;
    gNdsOpeningRoomHazeDObjDelta = 0;
    gNdsOpeningRoomHazeXObjDelta = 0;
    gNdsOpeningRoomHazeDisplaySet = 0;
    gNdsOpeningRoomSunlightAssetMask = 0;
    gNdsOpeningRoomSunlightDisplayListOffset = 0;
    gNdsOpeningRoomSunlightCreateResult = 0;
    gNdsOpeningRoomSunlightCreateMask = 0;
    gNdsOpeningRoomSunlightCreateGObjCount = 0;
    gNdsOpeningRoomSunlightGObjDelta = 0;
    gNdsOpeningRoomSunlightDObjDelta = 0;
    gNdsOpeningRoomSunlightXObjDelta = 0;
    gNdsOpeningRoomSunlightDisplaySet = 0;
    gNdsOpeningRoomDeskAssetMask = 0;
    gNdsOpeningRoomDeskDObjOffset = 0;
    gNdsOpeningRoomDeskCreateResult = 0;
    gNdsOpeningRoomDeskCreateMask = 0;
    gNdsOpeningRoomDeskCreateGObjCount = 0;
    gNdsOpeningRoomDeskGObjDelta = 0;
    gNdsOpeningRoomDeskDObjDelta = 0;
    gNdsOpeningRoomDeskXObjDelta = 0;
    gNdsOpeningRoomDeskDisplaySet = 0;
    gNdsOpeningRoomSunlightEjectResult = 0;
    gNdsOpeningRoomSunlightEjectBeforeGObjCount = 0;
    gNdsOpeningRoomSunlightEjectAfterGObjCount = 0;
    gNdsOpeningRoomSunlightEjectUnlinkedMask = 0;
    gNdsOpeningRoomOverlayCreateResult = 0;
    gNdsOpeningRoomOverlayDisplaySet = 0;
    gNdsOpeningRoomOverlayAlphaInit = 0;
    gNdsOpeningRoomOverlayCreateGObjCount = 0;
    gNdsOpeningRoomOverlayEjectResult = 0;
    gNdsOpeningRoomOverlayEjectBeforeGObjCount = 0;
    gNdsOpeningRoomOverlayEjectAfterGObjCount = 0;
    gNdsOpeningRoomOverlayEjectUnlinkedMask = 0;
    gNdsOpeningRoomCloseUpOverlayCreateResult = 0;
    gNdsOpeningRoomCloseUpOverlayCreateMask = 0;
    gNdsOpeningRoomCloseUpOverlayCreateTick = 0;
    gNdsOpeningRoomCloseUpOverlayCreateGObjCount = 0;
    gNdsOpeningRoomCloseUpOverlayGObjDelta = 0;
    gNdsOpeningRoomCloseUpOverlayDisplaySet = 0;
    gNdsOpeningRoomCloseUpOverlayAlphaInit = 0xffffffffu;
    gNdsOpeningRoomSpotlightAssetMask = 0;
    gNdsOpeningRoomSpotlightDisplayListOffset = 0;
    gNdsOpeningRoomSpotlightMObjOffset = 0;
    gNdsOpeningRoomSpotlightMatAnimOffset = 0;
    gNdsOpeningRoomSpotlightCreateResult = 0;
    gNdsOpeningRoomSpotlightCreateMask = 0;
    gNdsOpeningRoomSpotlightCreateTick = 0;
    gNdsOpeningRoomSpotlightCreateGObjCount = 0;
    gNdsOpeningRoomSpotlightGObjDelta = 0;
    gNdsOpeningRoomSpotlightDObjDelta = 0;
    gNdsOpeningRoomSpotlightXObjDelta = 0;
    gNdsOpeningRoomSpotlightMObjDelta = 0;
    gNdsOpeningRoomSpotlightAObjDelta = 0;
    gNdsOpeningRoomSpotlightDisplaySet = 0;
    gNdsOpeningRoomSpotlightProcessSet = 0;
    gNdsOpeningRoomSpotlightMObjSet = 0;
    gNdsOpeningRoomSpotlightMatAnimSet = 0;
    gNdsOpeningRoomSpotlightPositionSet = 0;
    gNdsOpeningRoomScene1CameraCreateResult = 0;
    gNdsOpeningRoomScene1CameraCreateMask = 0;
    gNdsOpeningRoomScene1CameraCreateGObjCount = 0;
    gNdsOpeningRoomScene1CameraGObjDelta = 0;
    gNdsOpeningRoomScene1CameraCObjDelta = 0;
    gNdsOpeningRoomScene1CameraXObjDelta = 0;
    gNdsOpeningRoomScene1CameraAObjDelta = 0;
    gNdsOpeningRoomScene1CameraDisplaySet = 0;
    gNdsOpeningRoomScene1CameraProcessSet = 0;
    gNdsOpeningRoomScene1CameraAnimSet = 0;
    gNdsOpeningRoomScene1CameraViewportSet = 0;
    gNdsOpeningRoomScene1CameraDLBufferSet = 0;
    gNdsOpeningRoomScene2CameraAssetMask = 0;
    gNdsOpeningRoomScene2CameraAnimOffset = 0;
    gNdsOpeningRoomScene2CameraEjectResult = 0;
    gNdsOpeningRoomScene2CameraEjectMask = 0;
    gNdsOpeningRoomScene2CameraEjectBeforeGObjCount = 0;
    gNdsOpeningRoomScene2CameraEjectAfterGObjCount = 0;
    gNdsOpeningRoomScene2CameraEjectBeforeCameraCount = 0;
    gNdsOpeningRoomScene2CameraEjectAfterCameraCount = 0;
    gNdsOpeningRoomScene2CameraCreateResult = 0;
    gNdsOpeningRoomScene2CameraCreateMask = 0;
    gNdsOpeningRoomScene2CameraCreateGObjCount = 0;
    gNdsOpeningRoomScene2CameraGObjDelta = 0;
    gNdsOpeningRoomScene2CameraCObjDelta = 0;
    gNdsOpeningRoomScene2CameraXObjDelta = 0;
    gNdsOpeningRoomScene2CameraAObjDelta = 0;
    gNdsOpeningRoomScene2CameraDisplaySet = 0;
    gNdsOpeningRoomScene2CameraProcessSet = 0;
    gNdsOpeningRoomScene2CameraAnimSet = 0;
    gNdsOpeningRoomScene2CameraViewportSet = 0;
    gNdsOpeningRoomScene2CameraDLBufferSet = 0;
    gNdsOpeningRoomCloseUpOverlayCameraCreateResult = 0;
    gNdsOpeningRoomCloseUpOverlayCameraCreateMask = 0;
    gNdsOpeningRoomCloseUpOverlayCameraCreateGObjCount = 0;
    gNdsOpeningRoomCloseUpOverlayCameraGObjDelta = 0;
    gNdsOpeningRoomCloseUpOverlayCameraCObjDelta = 0;
    gNdsOpeningRoomCloseUpOverlayCameraXObjDelta = 0;
    gNdsOpeningRoomCloseUpOverlayCameraDisplaySet = 0;
    gNdsOpeningRoomCloseUpOverlayCameraViewportSet = 0;
    gNdsOpeningRoomWallpaperCameraCreateResult = 0;
    gNdsOpeningRoomWallpaperCameraCreateMask = 0;
    gNdsOpeningRoomWallpaperCameraCreateGObjCount = 0;
    gNdsOpeningRoomWallpaperCameraGObjDelta = 0;
    gNdsOpeningRoomWallpaperCameraCObjDelta = 0;
    gNdsOpeningRoomWallpaperCameraXObjDelta = 0;
    gNdsOpeningRoomWallpaperCameraDisplaySet = 0;
    gNdsOpeningRoomWallpaperCameraViewportSet = 0;
    gNdsOpeningRoomLogoCameraAssetMask = 0;
    gNdsOpeningRoomLogoCameraAnimOffset = 0;
    gNdsOpeningRoomLogoCameraCreateResult = 0;
    gNdsOpeningRoomLogoCameraCreateMask = 0;
    gNdsOpeningRoomLogoCameraCreateGObjCount = 0;
    gNdsOpeningRoomLogoCameraGObjDelta = 0;
    gNdsOpeningRoomLogoCameraCObjDelta = 0;
    gNdsOpeningRoomLogoCameraXObjDelta = 0;
    gNdsOpeningRoomLogoCameraAObjDelta = 0;
    gNdsOpeningRoomLogoCameraDisplaySet = 0;
    gNdsOpeningRoomLogoCameraProcessSet = 0;
    gNdsOpeningRoomLogoCameraAnimSet = 0;
    gNdsOpeningRoomLogoCameraViewportSet = 0;
    gNdsOpeningRoomLogoAssetMask = 0;
    gNdsOpeningRoomLogoDObjOffset = 0;
    gNdsOpeningRoomLogoMObjOffset = 0;
    gNdsOpeningRoomLogoMatAnimOffset = 0;
    gNdsOpeningRoomLogoCreateResult = 0;
    gNdsOpeningRoomLogoCreateMask = 0;
    gNdsOpeningRoomLogoCreateGObjCount = 0;
    gNdsOpeningRoomLogoGObjDelta = 0;
    gNdsOpeningRoomLogoDObjDelta = 0;
    gNdsOpeningRoomLogoXObjDelta = 0;
    gNdsOpeningRoomLogoMObjDelta = 0;
    gNdsOpeningRoomLogoAObjDelta = 0;
    gNdsOpeningRoomLogoDisplaySet = 0;
    gNdsOpeningRoomLogoMObjSet = 0;
    gNdsOpeningRoomLogoMatAnimSet = 0;
    gNdsOpeningRoomLogoEjectResult = 0;
    gNdsOpeningRoomLogoEjectBeforeGObjCount = 0;
    gNdsOpeningRoomLogoEjectAfterGObjCount = 0;
    gNdsOpeningRoomLogoEjectUnlinkedMask = 0;
    gNdsOpeningRoomBossShadowAssetMask = 0;
    gNdsOpeningRoomBossShadowDisplayListOffset = 0;
    gNdsOpeningRoomBossShadowAnimOffset = 0;
    gNdsOpeningRoomBossShadowCreateResult = 0;
    gNdsOpeningRoomBossShadowCreateMask = 0;
    gNdsOpeningRoomBossShadowCreateGObjCount = 0;
    gNdsOpeningRoomBossShadowGObjDelta = 0;
    gNdsOpeningRoomBossShadowDObjDelta = 0;
    gNdsOpeningRoomBossShadowXObjDelta = 0;
    gNdsOpeningRoomBossShadowAObjDelta = 0;
    gNdsOpeningRoomBossShadowProcessSet = 0;
    gNdsOpeningRoomBossShadowDisplaySet = 0;
    gNdsOpeningRoomBossShadowAnimSet = 0;
    gNdsOpeningRoomBossShadowEjectResult = 0;
    gNdsOpeningRoomBossShadowEjectBeforeGObjCount = 0;
    gNdsOpeningRoomBossShadowEjectAfterGObjCount = 0;
    gNdsOpeningRoomBossShadowEjectUnlinkedMask = 0;
    gNdsOpeningRoomPencilsCreateResult = 0;
    gNdsOpeningRoomPencilsCreateMask = 0;
    gNdsOpeningRoomPencilsGObjDelta = 0;
    gNdsOpeningRoomPencilsDObjDelta = 0;
    gNdsOpeningRoomPencilsXObjDelta = 0;
    gNdsOpeningRoomPencilsAObjDelta = 0;
    gNdsOpeningRoomPencilsProcessSet = 0;
    gNdsOpeningRoomPencilsDisplaySet = 0;
    gNdsOpeningRoomPencilsDObjTreeCount = 0;
    gNdsOpeningRoomPencilsAnimRootCount = 0;
    gNdsOpeningRoomControllerCheckCount = 0;
    gNdsOpeningRoomPulledFighterKind = 0xffffffffu;
    gNdsOpeningRoomDroppedFighterKind = 0xffffffffu;
    gNdsOpeningRoomSkipToTitleCount = 0;
    gNdsOpeningRoomRelocResult = 0;
    gNdsOpeningRoomRelocInitCount = 0;
    gNdsOpeningRoomRelocLoadCount = 0;
    gNdsOpeningRoomRelocFileMask = 0;
    gNdsOpeningRoomRelocHeaderMask = 0;
    gNdsOpeningRoomRelocPayloadMask = 0;
    gNdsOpeningRoomRelocContentReady = 0;
    gNdsOpeningRoomRelocFixupReady = 0;
    gNdsOpeningRoomRelocBytesLoaded = 0;
    gNdsOpeningRoomRelocLastFileID = 0;
    gNdsOpeningRoomRelocLastSize = 0;
    gNdsOpeningRoomRelocWordSwapMask = 0;
    gNdsOpeningRoomRelocWordSwapCount = 0;
    gNdsOpeningRoomRelocWordSwapFailCount = 0;
    gNdsOpeningRoomRelocPointerFixupMask = 0;
    gNdsOpeningRoomRelocPointerFixupCount = 0;
    gNdsOpeningRoomRelocPointerFixupFailCount = 0;
    gNdsOpeningRoomRelocSymbolResolveCount = 0;
    gNdsOpeningRoomRelocSymbolResolveFailCount = 0;
    gNdsOpeningRoomRelocSymbolProbeMask = 0;
    gNdsOpeningRoomRelocLastSymbolOffset = 0;
    gNdsOpeningRoomRelocMObjSubNormalizeCount = 0;
    gNdsOpeningRoomRelocMObjSubNormalizeFailCount = 0;
    gNdsOpeningRoomRelocMObjSubFirstFlags = 0;
    gNdsOpeningRoomRelocMObjSubSourceResult = 0;
    gNdsOpeningRoomRelocMObjSubTextureFlagCount = 0;
    gNdsOpeningRoomRelocMObjSubZeroFlagCount = 0;
    gNdsOpeningRoomRelocMObjSubPrimColorCount = 0;
    gNdsOpeningRoomRelocMObjSubLightCount = 0;
    gNdsOpeningRoomRelocMObjSubFirstTextureOffset = 0xffffffffu;
    gNdsOpeningRoomRelocMObjSubFirstTextureFlags = 0;

    ndsPlatformClearOriginalSpritePreview();
    sNdsRelocInitCount = 0;
    sNdsFadeCreateCount = 0;
    sNdsOpeningRoomPencilsCountsCaptured = 0;
    sNdsOpeningRoomPencilsGObjsBefore = 0;
    sNdsOpeningRoomPencilsDObjsBefore = 0;
    sNdsOpeningRoomPencilsXObjsBefore = 0;
    sNdsOpeningRoomPencilsAObjsBefore = 0;
    sNdsOpeningRoomCloseUpOverlayCountsCaptured = 0;
    sNdsOpeningRoomCloseUpOverlayGObjsBefore = 0;
    sNdsOpeningRoomOutsideCountsCaptured = 0;
    sNdsOpeningRoomOutsideGObjsBefore = 0;
    sNdsOpeningRoomOutsideDObjsBefore = 0;
    sNdsOpeningRoomOutsideXObjsBefore = 0;
    sNdsOpeningRoomHazeCountsCaptured = 0;
    sNdsOpeningRoomHazeGObjsBefore = 0;
    sNdsOpeningRoomHazeDObjsBefore = 0;
    sNdsOpeningRoomHazeXObjsBefore = 0;
    sNdsOpeningRoomSunlightCountsCaptured = 0;
    sNdsOpeningRoomSunlightGObjsBefore = 0;
    sNdsOpeningRoomSunlightDObjsBefore = 0;
    sNdsOpeningRoomSunlightXObjsBefore = 0;
    sNdsOpeningRoomDeskCountsCaptured = 0;
    sNdsOpeningRoomDeskGObjsBefore = 0;
    sNdsOpeningRoomDeskDObjsBefore = 0;
    sNdsOpeningRoomDeskXObjsBefore = 0;
    sNdsOpeningRoomSpotlightCountsCaptured = 0;
    sNdsOpeningRoomSpotlightGObjsBefore = 0;
    sNdsOpeningRoomSpotlightDObjsBefore = 0;
    sNdsOpeningRoomSpotlightXObjsBefore = 0;
    sNdsOpeningRoomSpotlightMObjsBefore = 0;
    sNdsOpeningRoomSpotlightAObjsBefore = 0;
    sNdsOpeningRoomBossShadowCountsCaptured = 0;
    sNdsOpeningRoomBossShadowGObjsBefore = 0;
    sNdsOpeningRoomBossShadowDObjsBefore = 0;
    sNdsOpeningRoomBossShadowXObjsBefore = 0;
    sNdsOpeningRoomBossShadowAObjsBefore = 0;
    sNdsOpeningRoomScene1CameraCountsCaptured = 0;
    sNdsOpeningRoomScene1CameraGObjsBefore = 0;
    sNdsOpeningRoomScene1CameraCObjsBefore = 0;
    sNdsOpeningRoomScene1CameraXObjsBefore = 0;
    sNdsOpeningRoomScene1CameraAObjsBefore = 0;
    sNdsOpeningRoomScene2EjectMainCameraGObj = NULL;
    sNdsOpeningRoomScene2EjectFighterCameraGObj = NULL;
    sNdsOpeningRoomScene2CameraEjectCaptured = 0;
    sNdsOpeningRoomScene2CameraCountsCaptured = 0;
    sNdsOpeningRoomScene2CameraGObjsBefore = 0;
    sNdsOpeningRoomScene2CameraCObjsBefore = 0;
    sNdsOpeningRoomScene2CameraXObjsBefore = 0;
    sNdsOpeningRoomScene2CameraAObjsBefore = 0;
    sNdsOpeningRoomCloseUpOverlayCameraCountsCaptured = 0;
    sNdsOpeningRoomCloseUpOverlayCameraGObjsBefore = 0;
    sNdsOpeningRoomCloseUpOverlayCameraCObjsBefore = 0;
    sNdsOpeningRoomCloseUpOverlayCameraXObjsBefore = 0;
    sNdsOpeningRoomWallpaperCameraCountsCaptured = 0;
    sNdsOpeningRoomWallpaperCameraGObjsBefore = 0;
    sNdsOpeningRoomWallpaperCameraCObjsBefore = 0;
    sNdsOpeningRoomWallpaperCameraXObjsBefore = 0;
    sNdsOpeningRoomLogoCameraCountsCaptured = 0;
    sNdsOpeningRoomLogoCameraGObjsBefore = 0;
    sNdsOpeningRoomLogoCameraCObjsBefore = 0;
    sNdsOpeningRoomLogoCameraXObjsBefore = 0;
    sNdsOpeningRoomLogoCameraAObjsBefore = 0;
    sNdsOpeningRoomLogoCountsCaptured = 0;
    sNdsOpeningRoomLogoGObjsBefore = 0;
    sNdsOpeningRoomLogoDObjsBefore = 0;
    sNdsOpeningRoomLogoXObjsBefore = 0;
    sNdsOpeningRoomLogoMObjsBefore = 0;
    sNdsOpeningRoomLogoAObjsBefore = 0;
}

extern void ndsMNVSModeRunStartTransitionProbe(void);
extern void ndsMNPlayersVSRunReadyTransitionProbe(void);
extern void ndsMNMapsRunSelectVSBattleProbe(void);
extern void scVSBattleFuncUpdate(void);
extern void ndsFighterMarioFoxRunImmediateProofChain(void);
extern void ndsFighterMarioFoxSchedulerLoopPrepare(void);
extern void ndsFighterMarioFoxControllerLoopPrepare(void);
extern void ndsFighterMarioFoxPreviewLoopPrepare(void);
extern void ndsFighterMarioFoxGCRunAllLoopPrepare(void);
extern void ndsFighterMarioFoxLivePreviewPrepare(void);
extern u32 ndsSceneMipCacheHoldLogic(void);

#define NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX 180u
#define NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX 200u
#define NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX 220u
#define NDS_FIGHTER_GCRUNALL_LOOP_UPDATE_MAX 240u
/* The natural combat chain (wait/walk/dash-run/brake/turn/approach/attack/
 * damage/guard) needs more scripted frames than the old wait+walk proof. */
#define NDS_FIGHTER_NATURAL_MOTION_UPDATE_MAX 2400u
/* Remaining specials add long original recovery windows, especially Mario's
 * Super Jump Punch fall-special landing path. */
#define NDS_FIGHTER_BATTLE_PLAYABLE_UPDATE_MAX 9000u
#define NDS_FIGHTER_BATTLE_PLAYABLE_REALTIME_SMOKE_UPDATE_MAX 600u
#define NDS_FIGHTER_BATTLE_PLAYABLE_LIVE_UPDATE_MAX 216000u
#define NDS_FIGHTER_GCDRAWALL_LOOP_UPDATE_MAX 240u
#define NDS_FIGHTER_LIVE_PREVIEW_IDLE_UPDATE_MAX 60u
#define NDS_FIGHTER_LIVE_PREVIEW_DEV_UPDATE_MAX 3600u
#define NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT 2u
#define NDS_BATTLE_PLAYABLE_PRESENT_VBLANKS 2u
#define NDS_BATTLE_PLAYABLE_EARLY_COMBAT_TICKS 1800u

static u32 sNdsBattlePlayablePacingStartTick;
static u32 sNdsBattlePlayableLastPresentVBlank;
static u32 sNdsBattlePlayableLastDeadFrames;
static u32 sNdsBattlePlayableLastRebirthFrames;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
static u32 sNdsBattlePlayableProfileLoopStartTick;

static u32 ndsBattlePlayableProfileResidual(u32 total, u64 known)
{
    if (known > (u64)total)
    {
        gNdsRendererProfileConservationErrorTicks +=
            (u32)(known - (u64)total);
        return 0u;
    }
    return total - (u32)known;
}
#endif

#if NDS_RENDERER_M3_PHASE0_PROFILE
static void ndsRendererPhase05Reset(void)
{
    u32 calibration_tick;
    u32 calibration_index;

    gNdsRendererPhase05WallpaperSetupTicks = 0u;
    gNdsRendererPhase05WallpaperXMapTicks = 0u;
    gNdsRendererPhase05WallpaperYMapTicks = 0u;
    gNdsRendererPhase05WallpaperWriteTicks = 0u;
    gNdsRendererPhase05WallpaperCommitTicks = 0u;
    gNdsRendererPhase05PresentHardwareTicks = 0u;
    gNdsRendererPhase05GCDrawAllTicks = 0u;
    gNdsRendererPhase05StageTransitionTicks = 0u;
    gNdsRendererPhase05FighterWrapperTicks = 0u;
    gNdsRendererPhase05FrameResetTicks = 0u;
    gNdsRendererPhase05PresentTailTicks = 0u;
    gNdsRendererPhase05ProfileBookkeepingTicks = 0u;
    gNdsRendererPhase05ProfilePublishTicks = 0u;
    gNdsRendererPhase05FlushPrepTicks = 0u;
    gNdsRendererPhase05TimerReadCount = 0u;
    gNdsRendererPhase05TimerSpanCount = 0u;
    gNdsRendererPhase05CalibrationTicks = 0u;
    gNdsRendererPhase05CalibrationIntervals = 16u;
    gNdsRendererPhase05WallpaperRowCount = 0u;
    gNdsRendererPhase05WallpaperPixelWriteCount = 0u;

    calibration_tick = NDS_RENDERER_PHASE05_TICK();
    for (calibration_index = 0u;
         calibration_index < gNdsRendererPhase05CalibrationIntervals;
         calibration_index++)
    {
        u32 next_tick = NDS_RENDERER_PHASE05_TICK();

        gNdsRendererPhase05CalibrationTicks += next_tick - calibration_tick;
        calibration_tick = next_tick;
    }
}
#endif

static void ndsBattlePlayableAdvanceFastLogicClock(void)
{
#if NDS_HARNESS_FAST_LOGIC
    /* BattleShip scheduler.c:1038-1043 advances one tic per retrace. The
     * verifier removes the wait, not that one-update/one-tic contract. */
    sySchedulerSetTicCount(sySchedulerGetTicCount() + 1u);
#endif
}

static void ndsBattlePlayableAdvanceRealtimeLogicClock(void)
{
    /* The original scheduler advances this clock once per VI retrace before
     * the corresponding 60 Hz source update. Mode 163 batches those updates,
     * so preserve that exact one-tic/one-update contract here. */
    sySchedulerSetTicCount(sySchedulerGetTicCount() + 1u);
}

static void ndsBattlePlayableRecordLifecycleTaskmanExit(void)
{
    gNdsSCVSBattleLifecycleTaskmanExitCount++;
    gNdsSCVSBattleLifecycleTaskmanStatus = (u32)sSYTaskmanStatus;
    gNdsSCVSBattleLifecycleTimeLimit = gSCManagerBattleState->time_limit;
    gNdsSCVSBattleLifecycleTimeRemain = gSCManagerBattleState->time_remain;
    gNdsSCVSBattleLifecycleTimePassed = gSCManagerBattleState->time_passed;
    gNdsSCVSBattleLifecycleGameStatus = gSCManagerBattleState->game_status;
}

static void ndsAudioBackendUpdate(void)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
    ndsAudioBgmUpdate();
    syAudioUpdateBGMState();
#endif
#if NDS_IMPORT_BATTLESHIP_AUDIO_FGM
    ndsAudioFgmUpdate();
#endif
}

static void ndsRunMarioFoxProofUpdate(volatile u32 *counter)
{
    u32 start = cpuGetTiming();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 phase_start = start;
#endif

    scVSBattleFuncUpdate();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileSourceUpdateTicks = cpuGetTiming() - phase_start;
    phase_start = cpuGetTiming();
#endif
    ndsAudioBackendUpdate();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileAudioUpdateTicks = cpuGetTiming() - phase_start;
#endif
    gNdsRendererProfileUpdateTicks = cpuGetTiming() - start;
    dSYTaskmanUpdateCount++;
    gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
    if (counter != NULL)
    {
        (*counter)++;
    }
    gNdsSCVSBattleOriginalUpdateCount++;
    gNdsSCVSBattleOriginalUpdateResult =
        NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
    gNdsSCVSBattleOriginalSetupMask |=
        NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;
}

static void ndsBattlePlayablePacingStart(u32 fast_logic)
{
    u32 phase;
    u32 vblank = ndsPlatformVBlankCount();

    sNdsBattlePlayablePacingStartTick = cpuGetTiming();
    gNdsBattlePlayablePacingResult = 0;
    gNdsBattlePlayablePacingMode = fast_logic;
#if (NDS_RENDERER_HW_TRIANGLES != 0) && (NDS_DEV_LIVE_INPUT_PREVIEW != 0)
    gNdsBuildModeCanonicalWord =
        (fast_logic == 0u) ? NDS_BUILD_MODE_CANO_WORD : 0u;
    gNdsBuildModeShippedWord =
        (fast_logic == 0u) ? NDS_BUILD_MODE_SHIP_WORD : 0u;
#else
    gNdsBuildModeCanonicalWord = 0;
    gNdsBuildModeShippedWord = 0;
#endif
    gNdsBuildModeFastWord =
        (fast_logic != 0u) ? NDS_BUILD_MODE_FAST_WORD : 0u;
    gNdsBattlePlayablePacingLogicFrames = 0;
    gNdsBattlePlayablePacingPresentedFrames = 0;
    gNdsBattlePlayablePacingDrawCalls = 0;
    gNdsBattlePlayablePacingTimerTicks = 0;
    gNdsBattlePlayablePacingPresentFpsX10 = 0;
    gNdsBattlePlayablePacingLogicFpsX10 = 0;
    gNdsBattlePlayablePacingVBlankStart = vblank;
    gNdsBattlePlayablePacingVBlanks = 0u;
    gNdsBattlePlayablePacingPresentIntervalMin = 0xffffffffu;
    gNdsBattlePlayablePacingPresentIntervalMax = 0u;
    gNdsBattlePlayablePacingCadenceViolationCount = 0u;
    for (phase = 0u;
         phase < NDS_BATTLE_PLAYABLE_PACING_PHASE_COUNT;
         phase++)
    {
        gNdsBattlePlayablePacingPhasePresentCount[phase] = 0u;
        gNdsBattlePlayablePacingPhaseSlipCount[phase] = 0u;
    }
    sNdsBattlePlayableLastPresentVBlank = vblank;
    sNdsBattlePlayableLastDeadFrames =
        gNdsFighterBattlePlayableDeadFrames;
    sNdsBattlePlayableLastRebirthFrames =
        gNdsFighterBattlePlayableRebirthDownFrames +
        gNdsFighterBattlePlayableRebirthStandFrames +
        gNdsFighterBattlePlayableRebirthWaitFrames;
#if NDS_RENDERER_HW_TRIANGLES
    if (fast_logic == 0u)
    {
        ndsPlatformSetOriginalSpriteOverlayEnabled(TRUE);
    }
#endif
}

static void ndsBattlePlayablePacingUpdate(void)
{
    u32 ticks = cpuGetTiming() - sNdsBattlePlayablePacingStartTick;

    gNdsBattlePlayablePacingTimerTicks = ticks;
    gNdsBattlePlayablePacingVBlanks =
        ndsPlatformVBlankCount() - gNdsBattlePlayablePacingVBlankStart;
    if (ticks != 0u)
    {
        gNdsBattlePlayablePacingPresentFpsX10 =
            (u32)(((u64)gNdsBattlePlayablePacingPresentedFrames *
                   BUS_CLOCK * 10u) / ticks);
        gNdsBattlePlayablePacingLogicFpsX10 =
            (u32)(((u64)gNdsBattlePlayablePacingLogicFrames *
                   BUS_CLOCK * 10u) / ticks);
    }
    /* Short samples still need enough completed presentations to distinguish
     * sustained cadence from startup. Lifecycle verification resets this
     * epoch after its synchronized MATCH_START debugger stop. */
    if ((gNdsBattlePlayablePacingPresentedFrames >= 180u) ||
        (gNdsBattlePlayablePacingMode != 0u))
    {
        gNdsBattlePlayablePacingResult = NDS_BATTLE_PLAYABLE_PACING_PASS;
    }
}

static void ndsBattlePlayablePacingFinish(void)
{
    ndsBattlePlayablePacingUpdate();
    if (gNdsBattlePlayablePacingPresentIntervalMin == 0xffffffffu)
    {
        gNdsBattlePlayablePacingPresentIntervalMin = 0u;
    }
    gNdsBattlePlayablePacingResult = NDS_BATTLE_PLAYABLE_PACING_PASS;
}

static u32 ndsBattlePlayablePacingPhase(void)
{
    u32 rebirth_frames =
        gNdsFighterBattlePlayableRebirthDownFrames +
        gNdsFighterBattlePlayableRebirthStandFrames +
        gNdsFighterBattlePlayableRebirthWaitFrames;

    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->game_status == nSCBattleGameStatusWait))
    {
        return NDS_BATTLE_PLAYABLE_PACING_PHASE_COUNTDOWN;
    }
    if (gSCManagerBattleState->game_status != nSCBattleGameStatusGo)
    {
        return NDS_BATTLE_PLAYABLE_PACING_PHASE_RESULTS;
    }
    if ((gNdsFighterBattlePlayableDeadFrames !=
         sNdsBattlePlayableLastDeadFrames) ||
        (rebirth_frames != sNdsBattlePlayableLastRebirthFrames))
    {
        return NDS_BATTLE_PLAYABLE_PACING_PHASE_KO_REBIRTH;
    }
    if (gSCManagerBattleState->time_passed <
        NDS_BATTLE_PLAYABLE_EARLY_COMBAT_TICKS)
    {
        return NDS_BATTLE_PLAYABLE_PACING_PHASE_EARLY_COMBAT;
    }
    return NDS_BATTLE_PLAYABLE_PACING_PHASE_LATE_COMBAT;
}

void __attribute__((noinline, used)) ndsBattlePlayableFrameCompleteMarker(void)
{
    __asm__ volatile("" ::: "memory");
}

static void ndsBattlePlayablePresentFrame(void)
{
    u32 start = cpuGetTiming();
    u32 draw_start;
    u32 hud_start;
    u32 logic_tick;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 phase_start;
    u64 known;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    u32 phase05_start;
#endif

#if NDS_RENDERER_M3_PHASE0_PROFILE
    ndsRendererPhase05Reset();
    phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif
    gNdsRendererProfileFrameCount++;
    ndsRendererProfileFrameBegin();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileBeginFrameTicks = 0u;
    gNdsRendererProfileWallpaperTicks = 0u;
    gNdsRendererProfileForegroundTicks = 0u;
    gNdsRendererProfileStageLayer0Ticks = 0u;
    gNdsRendererProfileFlushTicks = 0u;
    gNdsRendererProfileVBlankWaitTicks = 0u;
    gNdsRendererProfilePostVBlankTicks = 0u;
    gNdsRendererProfileThreadTicks = 0u;
    gNdsRendererProfilePresentActiveTicks = 0u;
    gNdsRendererProfileDrawResidualTicks = 0u;
    gNdsRendererProfilePresentResidualTicks = 0u;
    gNdsRendererProfileLoopResidualTicks = 0u;
    gNdsRendererProfileConservationErrorTicks = 0u;
    gNdsRendererProfileDrawTicks = 0;
    gNdsRendererProfileHudTicks = 0;
    gNdsRendererProfileStageAdapterTicks = 0;
    gNdsRendererProfileMaterialTicks = 0;
    gNdsRendererProfileMatrixTicks = 0;
    gNdsRendererProfileDLTicks = 0;
    gNdsRendererProfileTextureTicks = 0;
    gNdsRendererProfileTextureConvertTicks = 0;
    gNdsRendererProfileTextureUploadTicks = 0;
    gNdsRendererProfileTextureUploads = 0;
    gNdsRendererProfileTextureUploadBytes = 0;
    gNdsRendererProfileTextureBinds = 0;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileTextureSourceTexels = 0;
    gNdsRendererProfileTextureGreenTexels = 0;
    gNdsRendererProfileTextureNonWhiteTexels = 0;
    gNdsRendererProfileTexturedVertexCount = 0;
    gNdsRendererProfileTextureSampleCount = 0;
    gNdsRendererProfileTextureSampleGreenCount = 0;
    gNdsRendererProfileTextureSampleNonWhiteCount = 0;
    gNdsRendererProfileTextureCacheAliasAvoidCount = 0;
    gNdsRendererProfileTexel1CompositeCount = 0;
    gNdsRendererProfileTexel1LoadMatchCount = 0;
    gNdsRendererProfileTexel1RejectCount = 0;
    gNdsRendererProfileTexel1RejectReasonMask = 0;
    gNdsRendererProfileTexel1LastFraction = 0;
    gNdsRendererProfileTexel1LastImage0 = 0;
    gNdsRendererProfileTexel1LastImage1 = 0;
    gNdsRendererProfileTexel1LastTileState = 0;
    gNdsRendererProfileTexel1LastPrimaryState = 0;
    /* Refreshes and evictions are scene-lifetime cache health counters. The
     * canonical frozen-water path expects both to stay zero; static-off labs
     * retain them to diagnose the animated fallback and earlier evictions. */
    gNdsRendererProfileTextureCoordMinS = 32767;
    gNdsRendererProfileTextureCoordMaxS = -32768;
    gNdsRendererProfileTextureCoordMinT = 32767;
    gNdsRendererProfileTextureCoordMaxT = -32768;
    gNdsRendererProfileTextureLaneLayoutMask = 0;
    gNdsRendererProfileTextureLaneByteAccessCount = 0;
    gNdsRendererProfileTextureLaneHalfwordAccessCount = 0;
    gNdsRendererProfileTextureLaneByteFormatMask = 0;
    gNdsRendererProfileTextureLaneHalfwordFormatMask = 0;
    gNdsRendererProfileTextureLaneByteMap = 0;
    gNdsRendererProfileTextureLaneHalfwordMap = 0;
    gNdsRendererProfileUseTextureRejectNoStatsCount = 0;
    gNdsRendererProfileUseTextureRejectStateOffCount = 0;
    gNdsRendererProfileUseTextureRejectNoCombineCount = 0;
    gNdsRendererProfileUseTextureRejectPrimitiveDecalCount = 0;
    gNdsRendererProfileUseTextureRejectNoTexel0Count = 0;
    gNdsRendererProfileUseTextureImplicitOnCount = 0;
    gNdsRendererProfileUseTextureRejectFirstReason = 0;
    gNdsRendererProfileUseTextureRejectFirstFlags = 0;
    gNdsRendererProfileUseTextureRejectFirstW0 = 0;
    gNdsRendererProfileUseTextureRejectFirstW1 = 0;
    gNdsRendererProfileUseTextureRejectFirstGeometry = 0;
    gNdsRendererProfileLightColorCommands = 0;
    gNdsRendererProfileLightDirectionCommands = 0;
    gNdsRendererProfileLightFallbackCount = 0;
    gNdsRendererProfileHardwareVertices = 0;
    gNdsRendererProfileHardwareTriangles = 0;
    gNdsRendererProfileHardwareBatchBeginCount = 0;
    gNdsRendererProfileHardwareBatchReuseCount = 0;
    gNdsRendererProfileHardwareBatchEndCount = 0;
    gNdsRendererProfileTexturePrepareCount = 0;
    gNdsRendererProfileTexturePrepareReuseCount = 0;
    gNdsRendererProfileImmutableListCount = 0;
    gNdsRendererProfileTrustedCommandCount = 0;
    gNdsRendererProfileValidatedCommandCount = 0;
    gNdsRendererProfileTriangleRunReuseCount = 0;
    gNdsRendererProfileTriangleSubmitTicks = 0;
    gNdsRendererProfileVertexSubmitTicks = 0;
    gNdsRendererProfileCi4LutBuildCount = 0;
    gNdsRendererProfileCi4LutReuseCount = 0;
    gNdsRendererProfileCi4IndexCacheBuildCount = 0;
    gNdsRendererProfileCi4IndexCacheReuseCount = 0;
    gNdsRendererProfileCi4RepresentativePixelCount = 0;
    gNdsRendererProfileCi4ReusePixelCount = 0;
    gNdsRendererProfileHardwareOverLimit = 0;
    gNdsRendererProfileOracleSamples = 0;
    gNdsRendererProfileOracleMismatches = 0;
    gNdsRendererProfileOracleMaxDelta = 0;
    gNdsRendererProfileMatrixLoadCount = 0;
    gNdsRendererProfileCameraMatrixCacheHitCount = 0;
    gNdsRendererProfileCameraMatrixCacheMissCount = 0;
    gNdsRendererProfileCameraMatrixCacheOverflowCount = 0;
    gNdsRendererProfileDObjWorldCacheHitCount = 0;
    gNdsRendererProfileDObjWorldCacheMissCount = 0;
    gNdsRendererProfileDObjWorldCacheOverflowCount = 0;
    gNdsRendererProfileStageWorldPersistentHitCount = 0;
    gNdsRendererProfileStageWorldPersistentMissCount = 0;
    gNdsRendererProfileStageWorldPersistentRejectCount = 0;
    gNdsRendererProfileStageWorldPersistentOverflowCount = 0;
    gNdsRendererProfileStageWorldPersistentOracleSampleCount = 0;
    gNdsRendererProfileStageWorldPersistentOracleMismatchCount = 0;
    gNdsRendererProfileAffineMatrixSamples = 0;
    gNdsRendererProfileAffineMatrixMismatches = 0;
    gNdsRendererProfileAffineMatrixMaxDelta = 0;
    gNdsRendererProfileRawVertexMinX = 32767;
    gNdsRendererProfileRawVertexMaxX = -32768;
    gNdsRendererProfileRawVertexMinY = 32767;
    gNdsRendererProfileRawVertexMaxY = -32768;
    gNdsRendererProfileRawVertexMinZ = 32767;
    gNdsRendererProfileRawVertexMaxZ = -32768;
    gNdsRendererProfileHWVertexMinX = 32767;
    gNdsRendererProfileHWVertexMaxX = -32768;
    gNdsRendererProfileHWVertexMinY = 32767;
    gNdsRendererProfileHWVertexMaxY = -32768;
    gNdsRendererProfileHWVertexMinZ = 32767;
    gNdsRendererProfileHWVertexMaxZ = -32768;
    gNdsRendererProfileHWVertexSaturateCount = 0;
#endif

#if NDS_RENDERER_M3_PHASE0_PROFILE
    NDS_RENDERER_PHASE05_FINISH(
        gNdsRendererPhase05FrameResetTicks, phase05_start);
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 1
    phase_start = cpuGetTiming();
#endif
    NDS_FREEZE_DIAGNOSTICS_MARK(NDS_FREEZE_BREADCRUMB_DRAW_START);
    ndsPlatformBeginFrame();
    ndsSObjPreviewBeginFrame();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileBeginFrameTicks = cpuGetTiming() - phase_start;
#endif
    draw_start = cpuGetTiming();
#if NDS_RENDERER_HW_TRIANGLES
    ndsFighterMarioFoxStageGCDrawAllLoopPresentHardwareFrame();
#else
    gcDrawAll();
#endif
    ndsSObjPreviewEndFrame();
    gNdsRendererProfileDrawTicks = cpuGetTiming() - draw_start;
    gNdsBattlePlayablePacingDrawCalls++;
    hud_start = cpuGetTiming();
    ndsPlatformRenderDebugHud();
    gNdsRendererProfileHudTicks = cpuGetTiming() - hud_start;
    ndsPlatformEndFrame();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    phase_start = cpuGetTiming();
#endif
    logic_tick = sySchedulerGetTicCount();
    ndsOsPostVBlank();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfilePostVBlankTicks += cpuGetTiming() - phase_start;
    phase_start = cpuGetTiming();
#endif
    ndsOsRunThreads();
    /* sySchedulerVRetrace performs required VI/client work but also increments
     * the original shared tic. Realtime game time already advanced once per
     * source update, so presentation must not add another logic tic. */
    sySchedulerSetTicCount(logic_tick);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileThreadTicks = cpuGetTiming() - phase_start;
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif
    gNdsFrameCounter++;
    gNdsBattlePlayablePacingPresentedFrames++;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    NDS_RENDERER_PHASE05_FINISH(
        gNdsRendererPhase05PresentTailTicks, phase05_start);
#endif
    gNdsRendererProfilePresentTicks = cpuGetTiming() - start;
#if NDS_RENDERER_M3_PHASE0_PROFILE
    phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfilePresentActiveTicks =
        ndsBattlePlayableProfileResidual(
            gNdsRendererProfilePresentTicks,
            gNdsRendererProfileVBlankWaitTicks);
    known = (u64)gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exclusive_ticks +
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_MARIO].exclusive_ticks +
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_FOX].exclusive_ticks +
            gNdsRendererProfileWallpaperTicks +
            gNdsRendererProfileForegroundTicks;
    gNdsRendererProfileDrawResidualTicks =
        ndsBattlePlayableProfileResidual(gNdsRendererProfileDrawTicks,
                                         known);
    known = (u64)gNdsRendererProfileBeginFrameTicks +
            gNdsRendererProfileDrawTicks +
            gNdsRendererProfileHudTicks +
            gNdsRendererProfileFlushTicks +
            gNdsRendererProfilePostVBlankTicks +
            gNdsRendererProfileThreadTicks;
    gNdsRendererProfilePresentResidualTicks =
        ndsBattlePlayableProfileResidual(
            gNdsRendererProfilePresentActiveTicks, known);
    /* Publish the GX state only after the VBlank wait, scheduler retrace
     * notification, post-wait bookkeeping, and runnable thread work have all
     * completed. This is the bounded frame endpoint, not merely the return
     * from glFlush(). */
    ndsPlatformProfileSampleFrameBoundaryGXState();
#endif
#if NDS_RENDERER_M3_PHASE0_PROFILE
    NDS_RENDERER_PHASE05_FINISH(
        gNdsRendererPhase05ProfileBookkeepingTicks, phase05_start);
    phase05_start = NDS_RENDERER_PHASE05_TICK();
#endif
    ndsRendererProfileFramePublish();
#if NDS_RENDERER_M3_PHASE0_PROFILE
    NDS_RENDERER_PHASE05_FINISH(
        gNdsRendererPhase05ProfilePublishTicks, phase05_start);
#endif
    ndsBattlePlayablePacingUpdate();
}

static void ndsBattlePlayablePresentRealtimeFrame(void)
{
    u32 phase = ndsBattlePlayablePacingPhase();
    u32 target = sNdsBattlePlayableLastPresentVBlank +
                 NDS_BATTLE_PLAYABLE_PRESENT_VBLANKS;
    u32 actual;
    u32 interval;

    ndsPlatformSchedulePresentAtVBlank(target);
    ndsBattlePlayablePresentFrame();
    actual = ndsPlatformVBlankCount();
    interval = actual - sNdsBattlePlayableLastPresentVBlank;
    if (interval < NDS_BATTLE_PLAYABLE_PRESENT_VBLANKS)
    {
        gNdsBattlePlayablePacingCadenceViolationCount++;
    }
    if (interval < gNdsBattlePlayablePacingPresentIntervalMin)
    {
        gNdsBattlePlayablePacingPresentIntervalMin = interval;
    }
    if (interval > gNdsBattlePlayablePacingPresentIntervalMax)
    {
        gNdsBattlePlayablePacingPresentIntervalMax = interval;
    }
    gNdsBattlePlayablePacingPhasePresentCount[phase]++;
    if (interval > NDS_BATTLE_PLAYABLE_PRESENT_VBLANKS)
    {
        gNdsBattlePlayablePacingPhaseSlipCount[phase] +=
            interval - NDS_BATTLE_PLAYABLE_PRESENT_VBLANKS;
    }
    sNdsBattlePlayableLastPresentVBlank = actual;
    sNdsBattlePlayableLastDeadFrames =
        gNdsFighterBattlePlayableDeadFrames;
    sNdsBattlePlayableLastRebirthFrames =
        gNdsFighterBattlePlayableRebirthDownFrames +
        gNdsFighterBattlePlayableRebirthStandFrames +
        gNdsFighterBattlePlayableRebirthWaitFrames;
}

static void ndsBattlePlayableFinalizePresentedIteration(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u64 known;

    gNdsRendererProfileLogicTick = sySchedulerGetTicCount();
    gNdsRendererProfileLoopWallTicks =
        cpuGetTiming() - sNdsBattlePlayableProfileLoopStartTick;
    known = (u64)gNdsRendererProfileInputTicks +
            gNdsRendererProfileUpdateTicks +
            gNdsRendererProfilePresentActiveTicks +
            gNdsRendererProfileVBlankWaitTicks;
    gNdsRendererProfileLoopResidualTicks =
        ndsBattlePlayableProfileResidual(
            gNdsRendererProfileLoopWallTicks, known);
#endif
    ndsBattlePlayableFrameCompleteMarker();
    NDS_FREEZE_DIAGNOSTICS_HEARTBEAT();
}

static void ndsRunMarioFoxProcessPrerequisiteLoop(void)
{
    u32 i;

    ndsRunMarioFoxProofUpdate(NULL);
    ndsFighterMarioFoxRunImmediateProofChain();

    for (i = 1u; i < 8u; i++)
    {
        if (gNdsFighterMarioFoxProcessLoopResult ==
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_PASS)
        {
            break;
        }
        ndsRunMarioFoxProofUpdate(NULL);
        ndsFighterMarioFoxRunImmediateProofChain();
    }
}

static void __attribute__((unused)) ndsRunMarioFoxGCRunAllPrerequisiteLoops(void)
{
    u32 i;

    ndsRunMarioFoxProcessPrerequisiteLoop();

    ndsFighterMarioFoxSchedulerLoopPrepare();
    for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
    {
        ndsRunMarioFoxProofUpdate(&gNdsFighterSchedulerLoopTaskmanUpdateCount);
        if (gNdsFighterMarioFoxSchedulerLoopResult ==
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
        {
            break;
        }
    }

    ndsFighterMarioFoxControllerLoopPrepare();
    for (i = 0u; i < NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX; i++)
    {
        ndsRunMarioFoxProofUpdate(&gNdsFighterControllerLoopTaskmanUpdateCount);
        if (gNdsFighterMarioFoxControllerLoopResult ==
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS)
        {
            break;
        }
    }

    ndsFighterMarioFoxPreviewLoopPrepare();
    for (i = 0u; i < NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX; i++)
    {
        ndsRunMarioFoxProofUpdate(&gNdsFighterPreviewLoopTaskmanUpdateCount);
        if (gNdsFighterMarioFoxPreviewLoopResult ==
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS)
        {
            break;
        }
    }

    ndsFighterMarioFoxGCRunAllLoopPrepare();
    for (i = 0u; i < NDS_FIGHTER_GCRUNALL_LOOP_UPDATE_MAX; i++)
    {
        ndsRunMarioFoxProofUpdate(&gNdsFighterGCRunAllLoopTaskmanUpdateCount);
        if (gNdsFighterMarioFoxGCRunAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS)
        {
            break;
        }
    }
}

/* Diagnostic snapshot of the real object-manager state after
 * mnStartupFuncStart ran. Every value here is read from the original object
 * manager's pools/links, proving startup built real GObj/CObj/SObj state
 * through the original gcMake and gcAdd and lbCommon paths (not a hand-written
 * bridge). */
static void ndsCaptureRealStartupState(void)
{
    GObj *wallpaper_gobj;
    GObj *actor_gobj;
    GObj *wallpaper_camera_gobj = NULL;
    GObj *default_camera_gobj = NULL;
    SObj *logo_sobj = NULL;
    CObj *default_cobj = NULL;
    GObjProcess *proc = NULL;
    s32 link;

    gNdsStartupSkipAllowWait = (u32)sMNStartupSkipAllowWait;
    gNdsStartupProceedOpening = (u32)sMNStartupIsProceedOpening;

    /* Real object counts from the original object manager free-list counters. */
    gNdsStartupGObjCreateCount = sGCCommonsActiveNum;
    gNdsStartupCameraCreateCount = sGCCamerasActiveNum;
    gNdsStartupSpriteCreateCount = sGCSpritesActiveNum;
    gNdsStartupRelocInitCount = sNdsRelocInitCount;
    gNdsStartupFadeCreateCount = sNdsFadeCreateCount;

    /* Find the real startup GObjs through the original object manager. The
     * actor is id 0 (gcMakeGObjSPAfter(0, ...)); the wallpaper and wallpaper
     * camera use the kind enums the original scene set. */
    actor_gobj = gcFindGObjByID(0);
    wallpaper_gobj = gcFindGObjByID(nGCCommonKindWallpaper);
    wallpaper_camera_gobj = gcFindGObjByID(nGCCommonKindWallpaperCamera);

    /* The default camera is created with id 0xffffffff via gcMakeDefaultCameraGObj;
     * locate it by scanning the camera link for the non-wallpaper CObj GObj. */
    for (link = 0; link < (s32)GC_COMMON_MAX_LINKS; link++)
    {
        GObj *gobj = gGCCommonLinks[link];
        while (gobj != NULL)
        {
            if (gobj->obj_kind == nGCCommonAppendCamera &&
                gobj->id != nGCCommonKindWallpaperCamera &&
                default_camera_gobj == NULL)
            {
                default_camera_gobj = gobj;
            }
            gobj = gobj->link_next;
        }
    }

    if (actor_gobj != NULL)
    {
        gNdsStartupActorFuncSet = (actor_gobj->func_run == mnStartupActorFuncRun) ? 1u : 0u;
    }
    if (wallpaper_gobj != NULL)
    {
        logo_sobj = SObjGetStruct(wallpaper_gobj);
        proc = wallpaper_gobj->gobjproc_head;
        gNdsStartupWallpaperDisplaySet =
            (wallpaper_gobj->proc_display == lbCommonDrawSObjAttr) &&
            (wallpaper_gobj->dl_link_id == 0) ? 1u : 0u;
    }
    if (logo_sobj != NULL)
    {
        gNdsStartupWallpaperParentValid =
            (logo_sobj->parent_gobj == wallpaper_gobj) ? 1u : 0u;
        gNdsStartupLogoPosX = (u32)(s32)logo_sobj->pos.x;
        gNdsStartupLogoPosY = (u32)(s32)logo_sobj->pos.y;
        gNdsStartupLogoFastcopyCleared =
            ((logo_sobj->sprite.attr & SP_FASTCOPY) == 0) ? 1u : 0u;
    }
    if (proc != NULL)
    {
        gNdsStartupWallpaperProcessKind = (u32)proc->kind;
        gNdsStartupWallpaperProcessPriority = (u32)proc->priority;
    }
    if (wallpaper_camera_gobj != NULL)
    {
        gNdsStartupWallpaperCameraMaskLow =
            (u32)(wallpaper_camera_gobj->camera_mask & 0xFFFFFFFFu);
    }
    if (default_camera_gobj != NULL)
    {
        default_cobj = CObjGetStruct(default_camera_gobj);
        gNdsStartupDefaultCameraColor = default_cobj->color;
    }
}

static void ndsCapturePostUpdateStartupState(void)
{
    GObj *wallpaper_gobj = gcFindGObjByID(nGCCommonKindWallpaper);

    gNdsTaskmanPostUpdateOpening = (u32)sMNStartupIsProceedOpening;
    gNdsTaskmanPostUpdateSceneKind = gSCManagerSceneData.scene_curr;
    gNdsTaskmanPostUpdateScenePrev = gSCManagerSceneData.scene_prev;
    gNdsTaskmanPostUpdateStatus = (u32)sSYTaskmanStatus;
    gNdsTaskmanPostUpdateGObjCount = sGCCommonsActiveNum;
    gNdsTaskmanPostUpdateFadeCount = sNdsFadeCreateCount;

    if (wallpaper_gobj != NULL)
    {
        SObj *logo_sobj = SObjGetStruct(wallpaper_gobj);

        if (logo_sobj != NULL)
        {
            gNdsTaskmanPostUpdateLogoPosX = (u32)(s32)logo_sobj->pos.x;
            gNdsTaskmanPostUpdateLogoPosY = (u32)(s32)logo_sobj->pos.y;
        }
    }
}

static void ndsRunBoundedTaskmanUpdates(struct SYTaskFunction *tfunc)
{
    u32 i;

    if ((tfunc == NULL) || (tfunc->task_update == NULL))
    {
        return;
    }
    for (i = 0; i < NDS_STARTUP_BOUNDED_UPDATES; i++)
    {
        tfunc->task_update(tfunc);
        dSYTaskmanUpdateCount++;
        gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
        gNdsTaskmanPostUpdateSkip = (u32)sMNStartupSkipAllowWait;
        ndsCapturePostUpdateStartupState();

        if (((i + 1u) == NDS_STARTUP_LOGO_DRAW_UPDATE) &&
            (tfunc->scene_draw != NULL))
        {
            gNdsStartupLogoDrawUpdateCount = dSYTaskmanUpdateCount;
            tfunc->scene_draw();
            dSYTaskmanFrameCount++;
        }
        if (((i + 1u) >= NDS_STARTUP_LOGO_DRAW_UPDATE) &&
            (((i + 1u) == NDS_STARTUP_LOGO_DRAW_UPDATE) ||
             (((i + 1u) % NDS_STARTUP_PRESENT_INTERVAL) == 0u)))
        {
            ndsOpeningMoviePresentFrame();
        }
    }
}

void ndsOpeningRoomCapturePencilsCountsBefore(void)
{
    if ((sNdsOpeningRoomPencilsCountsCaptured != 0) ||
        (gNdsOpeningRoomFirstEventDataResult !=
         NDS_OPENING_ROOM_FIRST_EVENT_DATA_PASS))
    {
        return;
    }

    sNdsOpeningRoomPencilsGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomPencilsDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomPencilsXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomPencilsAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomPencilsCountsCaptured = 1;
}

void ndsOpeningRoomCapturePencilsCreation(void)
{
    u32 mask = 0;
    DObj *dobj;

    if ((gNdsOpeningRoomFirstEventDataResult !=
         NDS_OPENING_ROOM_FIRST_EVENT_DATA_PASS) ||
        (sNdsOpeningRoomPencilsCountsCaptured == 0) ||
        (sMVOpeningRoomPencilsGObj == NULL))
    {
        return;
    }

    gNdsOpeningRoomPencilsGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomPencilsGObjsBefore;
    gNdsOpeningRoomPencilsDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomPencilsDObjsBefore;
    gNdsOpeningRoomPencilsXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomPencilsXObjsBefore;
    gNdsOpeningRoomPencilsAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomPencilsAObjsBefore;

    if ((sMVOpeningRoomPencilsGObj != NULL) &&
        (gNdsOpeningRoomPencilsGObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_GOBJ_READY;
    }
    if (gNdsOpeningRoomPencilsDObjDelta == NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS)
    {
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomPencilsXObjDelta ==
        (NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS * 2u))
    {
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_XOBJ_READY;
    }
    if ((sMVOpeningRoomPencilsGObj != NULL) &&
        (sMVOpeningRoomPencilsGObj->gobjproc_head != NULL) &&
        (sMVOpeningRoomPencilsGObj->gobjproc_head->exec.func ==
         mvOpeningRoomCommonProcUpdate))
    {
        gNdsOpeningRoomPencilsProcessSet = 1;
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_PROCESS_READY;
    }
    if ((sMVOpeningRoomPencilsGObj != NULL) &&
        (sMVOpeningRoomPencilsGObj->proc_display == gcDrawDObjTreeForGObj) &&
        (sMVOpeningRoomPencilsGObj->dl_link_id == 6))
    {
        gNdsOpeningRoomPencilsDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_DISPLAY_READY;
    }

    gNdsOpeningRoomPencilsDObjTreeCount = 0;
    gNdsOpeningRoomPencilsAnimRootCount = 0;
    dobj = DObjGetStruct(sMVOpeningRoomPencilsGObj);
    while (dobj != NULL)
    {
        gNdsOpeningRoomPencilsDObjTreeCount++;
        if (dobj->is_anim_root != FALSE)
        {
            gNdsOpeningRoomPencilsAnimRootCount++;
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
    if ((gNdsOpeningRoomPencilsDObjTreeCount ==
         NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS) &&
        (gNdsOpeningRoomPencilsAnimRootCount == 1u))
    {
        mask |= NDS_OPENING_ROOM_PENCILS_CREATE_ANIM_ROOT_READY;
    }

    gNdsOpeningRoomPencilsCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_PENCILS_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_PENCILS_CREATE_READY_MASK)
    {
        gNdsOpeningRoomPencilsCreateResult =
            NDS_OPENING_ROOM_PENCILS_CREATE_PASS;
    }
}

static sb32 ndsOpeningRoomContainsCommonGObj(GObj *target)
{
    s32 link;

    if (target == NULL)
    {
        return FALSE;
    }
    for (link = 0; link < (s32)GC_COMMON_MAX_LINKS; link++)
    {
        GObj *gobj = gGCCommonLinks[link];

        while (gobj != NULL)
        {
            if (gobj == target)
            {
                return TRUE;
            }
            gobj = gobj->link_next;
        }
    }
    return FALSE;
}

static sb32 ndsOpeningRoomContainsDLGObj(GObj *target)
{
    s32 link;

    if (target == NULL)
    {
        return FALSE;
    }
    for (link = 0; link < (s32)GC_COMMON_MAX_DLLINKS; link++)
    {
        GObj *gobj = gGCCommonDLLinks[link];

        while (gobj != NULL)
        {
            if (gobj == target)
            {
                return TRUE;
            }
            gobj = gobj->dl_link_next;
        }
    }
    return FALSE;
}

void ndsOpeningRoomRecordOverlayEject(void *gobj)
{
    GObj *overlay_gobj = (GObj*)gobj;
    u32 mask = 0;

    if (overlay_gobj == NULL)
    {
        gNdsOpeningRoomOverlayEjectUnlinkedMask = 0;
        return;
    }

    if (!ndsOpeningRoomContainsCommonGObj(overlay_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_COMMON_UNLINKED;
    }
    if (!ndsOpeningRoomContainsDLGObj(overlay_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_DL_UNLINKED;
    }

    gNdsOpeningRoomOverlayEjectUnlinkedMask = mask;
    if ((mask & NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK) ==
        NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK)
    {
        gNdsOpeningRoomOverlayEjectResult =
            NDS_OPENING_ROOM_OVERLAY_EJECT_PASS;
    }
}

void ndsOpeningRoomRecordSunlightEject(void *gobj)
{
    GObj *sunlight_gobj = (GObj*)gobj;
    u32 mask = 0;

    if (sunlight_gobj == NULL)
    {
        gNdsOpeningRoomSunlightEjectUnlinkedMask = 0;
        return;
    }

    if (!ndsOpeningRoomContainsCommonGObj(sunlight_gobj))
    {
        mask |= NDS_OPENING_ROOM_SUNLIGHT_EJECT_COMMON_UNLINKED;
    }
    if (!ndsOpeningRoomContainsDLGObj(sunlight_gobj))
    {
        mask |= NDS_OPENING_ROOM_SUNLIGHT_EJECT_DL_UNLINKED;
    }

    gNdsOpeningRoomSunlightEjectUnlinkedMask = mask;
    if ((mask & NDS_OPENING_ROOM_SUNLIGHT_EJECT_UNLINKED_MASK) ==
        NDS_OPENING_ROOM_SUNLIGHT_EJECT_UNLINKED_MASK)
    {
        gNdsOpeningRoomSunlightEjectResult =
            NDS_OPENING_ROOM_SUNLIGHT_EJECT_PASS;
    }
}

void ndsOpeningRoomCaptureCloseUpOverlayCountsBefore(void)
{
    if (sNdsOpeningRoomCloseUpOverlayCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomCloseUpOverlayGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomCloseUpOverlayCountsCaptured = 1;
}

void ndsOpeningRoomCaptureCloseUpOverlayCreation(void *gobj)
{
    GObj *overlay_gobj = (GObj*)gobj;
    u32 mask = 0;

    if ((overlay_gobj == NULL) ||
        (sNdsOpeningRoomCloseUpOverlayCountsCaptured == 0))
    {
        return;
    }

    gNdsOpeningRoomCloseUpOverlayCreateTick =
        (u32)sMVOpeningRoomTotalTimeTics;
    gNdsOpeningRoomCloseUpOverlayCreateGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomCloseUpOverlayGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomCloseUpOverlayGObjsBefore;

    if (gNdsOpeningRoomCloseUpOverlayGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_GOBJ_READY;
    }
    if ((overlay_gobj->proc_display == mvOpeningRoomCloseUpOverlayProcDisplay) &&
        (overlay_gobj->dl_link_id == 26))
    {
        gNdsOpeningRoomCloseUpOverlayDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_DISPLAY_READY;
    }
    if (gNdsOpeningRoomCloseUpOverlayAlphaInit == 0u)
    {
        mask |= NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_ALPHA_READY;
    }

    gNdsOpeningRoomCloseUpOverlayCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_READY_MASK)
    {
        gNdsOpeningRoomCloseUpOverlayCreateResult =
            NDS_OPENING_ROOM_CLOSEUP_OVERLAY_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureSunlightCountsBefore(void)
{
    if (sNdsOpeningRoomSunlightCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomSunlightGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomSunlightDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomSunlightXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomSunlightCountsCaptured = 1;
}

void ndsOpeningRoomCaptureOutsideCountsBefore(void)
{
    if (sNdsOpeningRoomOutsideCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomOutsideGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomOutsideDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomOutsideXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomOutsideCountsCaptured = 1;
}

void ndsOpeningRoomCaptureHazeCountsBefore(void)
{
    if (sNdsOpeningRoomHazeCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomHazeGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomHazeDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomHazeXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomHazeCountsCaptured = 1;
}

void ndsOpeningRoomCaptureOutsideCreation(void *gobj)
{
    GObj *outside_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;

    if ((outside_gobj == NULL) ||
        (sNdsOpeningRoomOutsideCountsCaptured == 0))
    {
        return;
    }

    dobj = DObjGetStruct(outside_gobj);
    gNdsOpeningRoomOutsideCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomOutsideGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomOutsideGObjsBefore;
    gNdsOpeningRoomOutsideDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomOutsideDObjsBefore;
    gNdsOpeningRoomOutsideXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomOutsideXObjsBefore;

    if (gNdsOpeningRoomOutsideGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_OUTSIDE_CREATE_GOBJ_READY;
    }
    if ((dobj != NULL) && (gNdsOpeningRoomOutsideDObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_OUTSIDE_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomOutsideXObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_OUTSIDE_CREATE_XOBJ_READY;
    }
    if ((outside_gobj->proc_display == gcDrawDObjDLLinksForGObj) &&
        (outside_gobj->dl_link_id == 6))
    {
        gNdsOpeningRoomOutsideDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_OUTSIDE_CREATE_DISPLAY_READY;
    }

    gNdsOpeningRoomOutsideCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_OUTSIDE_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_OUTSIDE_CREATE_READY_MASK)
    {
        gNdsOpeningRoomOutsideCreateResult =
            NDS_OPENING_ROOM_OUTSIDE_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureHazeCreation(void *gobj)
{
    GObj *haze_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;

    if ((haze_gobj == NULL) ||
        (sNdsOpeningRoomHazeCountsCaptured == 0))
    {
        return;
    }

    dobj = DObjGetStruct(haze_gobj);
    gNdsOpeningRoomHazeCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomHazeGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomHazeGObjsBefore;
    gNdsOpeningRoomHazeDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomHazeDObjsBefore;
    gNdsOpeningRoomHazeXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomHazeXObjsBefore;

    if (gNdsOpeningRoomHazeGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_HAZE_CREATE_GOBJ_READY;
    }
    if ((dobj != NULL) && (gNdsOpeningRoomHazeDObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_HAZE_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomHazeXObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_HAZE_CREATE_XOBJ_READY;
    }
    if ((haze_gobj->proc_display == gcDrawDObjDLLinksForGObj) &&
        (haze_gobj->dl_link_id == 6))
    {
        gNdsOpeningRoomHazeDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_HAZE_CREATE_DISPLAY_READY;
    }

    gNdsOpeningRoomHazeCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_HAZE_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_HAZE_CREATE_READY_MASK)
    {
        gNdsOpeningRoomHazeCreateResult =
            NDS_OPENING_ROOM_HAZE_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureSunlightCreation(void *gobj)
{
    GObj *sunlight_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;

    if ((sunlight_gobj == NULL) ||
        (sNdsOpeningRoomSunlightCountsCaptured == 0))
    {
        return;
    }

    dobj = DObjGetStruct(sunlight_gobj);
    gNdsOpeningRoomSunlightCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomSunlightGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomSunlightGObjsBefore;
    gNdsOpeningRoomSunlightDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomSunlightDObjsBefore;
    gNdsOpeningRoomSunlightXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomSunlightXObjsBefore;

    if (gNdsOpeningRoomSunlightGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_SUNLIGHT_CREATE_GOBJ_READY;
    }
    if ((dobj != NULL) && (gNdsOpeningRoomSunlightDObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_SUNLIGHT_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomSunlightXObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_SUNLIGHT_CREATE_XOBJ_READY;
    }
    if ((sunlight_gobj->proc_display == gcDrawDObjDLLinksForGObj) &&
        (sunlight_gobj->dl_link_id == 6))
    {
        gNdsOpeningRoomSunlightDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_SUNLIGHT_CREATE_DISPLAY_READY;
    }

    gNdsOpeningRoomSunlightCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_SUNLIGHT_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_SUNLIGHT_CREATE_READY_MASK)
    {
        gNdsOpeningRoomSunlightCreateResult =
            NDS_OPENING_ROOM_SUNLIGHT_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureDeskCountsBefore(void)
{
    if (sNdsOpeningRoomDeskCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomDeskGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomDeskDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomDeskXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomDeskCountsCaptured = 1;
}

void ndsOpeningRoomCaptureDeskCreation(void *gobj)
{
    GObj *desk_gobj = (GObj*)gobj;
    u32 mask = 0;

    if ((desk_gobj == NULL) ||
        (sNdsOpeningRoomDeskCountsCaptured == 0))
    {
        return;
    }

    gNdsOpeningRoomDeskCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomDeskGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomDeskGObjsBefore;
    gNdsOpeningRoomDeskDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomDeskDObjsBefore;
    gNdsOpeningRoomDeskXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomDeskXObjsBefore;

    if (gNdsOpeningRoomDeskGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_DESK_CREATE_GOBJ_READY;
    }
    if (gNdsOpeningRoomDeskDObjDelta != 0u)
    {
        mask |= NDS_OPENING_ROOM_DESK_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomDeskXObjDelta != 0u)
    {
        mask |= NDS_OPENING_ROOM_DESK_CREATE_XOBJ_READY;
    }
    if ((desk_gobj->proc_display == gcDrawDObjTreeForGObj) &&
        (desk_gobj->dl_link_id == 6))
    {
        gNdsOpeningRoomDeskDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_DESK_CREATE_DISPLAY_READY;
    }

    gNdsOpeningRoomDeskCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_DESK_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_DESK_CREATE_READY_MASK)
    {
        gNdsOpeningRoomDeskCreateResult =
            NDS_OPENING_ROOM_DESK_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureSpotlightCountsBefore(void)
{
    if (sNdsOpeningRoomSpotlightCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomSpotlightGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomSpotlightDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomSpotlightXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomSpotlightMObjsBefore = sGCMaterialsActive;
    sNdsOpeningRoomSpotlightAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomSpotlightCountsCaptured = 1;
}

void ndsOpeningRoomCaptureSpotlightCreation(void *gobj)
{
    GObj *spotlight_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;

    if ((spotlight_gobj == NULL) ||
        (sNdsOpeningRoomSpotlightCountsCaptured == 0))
    {
        return;
    }

    dobj = DObjGetStruct(spotlight_gobj);
    gNdsOpeningRoomSpotlightCreateTick = (u32)sMVOpeningRoomTotalTimeTics;
    gNdsOpeningRoomSpotlightCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomSpotlightGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomSpotlightGObjsBefore;
    gNdsOpeningRoomSpotlightDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomSpotlightDObjsBefore;
    gNdsOpeningRoomSpotlightXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomSpotlightXObjsBefore;
    gNdsOpeningRoomSpotlightMObjDelta =
        sGCMaterialsActive - sNdsOpeningRoomSpotlightMObjsBefore;
    gNdsOpeningRoomSpotlightAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomSpotlightAObjsBefore;

    if (gNdsOpeningRoomSpotlightGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_GOBJ_READY;
    }
    if ((dobj != NULL) && (gNdsOpeningRoomSpotlightDObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomSpotlightXObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_XOBJ_READY;
    }
    if (gNdsOpeningRoomSpotlightMObjDelta >= 1u)
    {
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_MOBJ_READY;
    }
    if ((spotlight_gobj->proc_display == gcDrawDObjDLHead1) &&
        (spotlight_gobj->dl_link_id == 27))
    {
        gNdsOpeningRoomSpotlightDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_DISPLAY_READY;
    }
    if ((spotlight_gobj->gobjproc_head != NULL) &&
        (spotlight_gobj->gobjproc_head->exec.func == gcPlayAnimAll))
    {
        gNdsOpeningRoomSpotlightProcessSet = 1;
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_PROCESS_READY;
    }
    if ((dobj != NULL) && (dobj->mobj != NULL))
    {
        gNdsOpeningRoomSpotlightMObjSet = 1;
        if (dobj->mobj->matanim_joint.event32 != NULL)
        {
            gNdsOpeningRoomSpotlightMatAnimSet = 1;
            mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_MATANIM_READY;
        }
    }
    if ((dobj != NULL) &&
        (dobj->scale.vec.f.x > 0.0F) &&
        (dobj->scale.vec.f.y == 1.0F) &&
        (dobj->scale.vec.f.z > 0.0F) &&
        ((dobj->translate.vec.f.x != 0.0F) ||
         (dobj->translate.vec.f.y != 0.0F) ||
         (dobj->translate.vec.f.z != 0.0F)))
    {
        gNdsOpeningRoomSpotlightPositionSet = 1;
        mask |= NDS_OPENING_ROOM_SPOTLIGHT_CREATE_POSITION_READY;
    }

    gNdsOpeningRoomSpotlightCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_SPOTLIGHT_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_SPOTLIGHT_CREATE_READY_MASK)
    {
        gNdsOpeningRoomSpotlightCreateResult =
            NDS_OPENING_ROOM_SPOTLIGHT_CREATE_PASS;
    }
}

static sb32 ndsOpeningRoomCObjViewportIsSet(CObj *cobj)
{
    return ((cobj != NULL) &&
            (cobj->viewport.vp.vscale[0] == 600) &&
            (cobj->viewport.vp.vscale[1] == 440) &&
            (cobj->viewport.vp.vtrans[0] == 640) &&
            (cobj->viewport.vp.vtrans[1] == 480) &&
            (cobj->viewport.vp.vscale[2] == 511) &&
            (cobj->viewport.vp.vtrans[2] == 511));
}

void ndsOpeningRoomCaptureScene1CameraCountsBefore(void)
{
    if (sNdsOpeningRoomScene1CameraCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomScene1CameraGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomScene1CameraCObjsBefore = sGCCamerasActiveNum;
    sNdsOpeningRoomScene1CameraXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomScene1CameraAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomScene1CameraCountsCaptured = 1;
}

void ndsOpeningRoomCaptureScene1CameraCreation(void)
{
    GObj *main_gobj = sMVOpeningRoomMainCameraGObj;
    GObj *fighter_gobj = sMVOpeningRoomFighterCameraGObj;
    CObj *main_cobj;
    CObj *fighter_cobj;
    u32 mask = 0;

    if ((main_gobj == NULL) || (fighter_gobj == NULL) ||
        (sNdsOpeningRoomScene1CameraCountsCaptured == 0))
    {
        return;
    }

    main_cobj = CObjGetStruct(main_gobj);
    fighter_cobj = CObjGetStruct(fighter_gobj);
    gNdsOpeningRoomScene1CameraCreateGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomScene1CameraGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomScene1CameraGObjsBefore;
    gNdsOpeningRoomScene1CameraCObjDelta =
        sGCCamerasActiveNum - sNdsOpeningRoomScene1CameraCObjsBefore;
    gNdsOpeningRoomScene1CameraXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomScene1CameraXObjsBefore;
    gNdsOpeningRoomScene1CameraAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomScene1CameraAObjsBefore;

    if (gNdsOpeningRoomScene1CameraGObjDelta == 2u)
    {
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_MAIN_GOBJ_READY;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_FIGHTER_GOBJ_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        (gNdsOpeningRoomScene1CameraCObjDelta == 2u))
    {
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_COBJ_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        (gNdsOpeningRoomScene1CameraXObjDelta == 4u) &&
        (main_cobj->xobjs_num == 2) &&
        (fighter_cobj->xobjs_num == 2))
    {
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_XOBJ_READY;
    }
    if ((main_gobj->proc_display == func_80017EC0) &&
        (main_gobj->dl_link_priority == 80) &&
        (main_gobj->camera_mask == (1ULL << 6)) &&
        (fighter_gobj->proc_display == func_80017EC0) &&
        (fighter_gobj->dl_link_priority == 40) &&
        (fighter_gobj->camera_mask == ((1ULL << 27) | (1ULL << 9))))
    {
        gNdsOpeningRoomScene1CameraDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_DISPLAY_READY;
    }
    if ((main_gobj->gobjproc_head != NULL) &&
        (main_gobj->gobjproc_head->exec.func == gcPlayCamAnim) &&
        (fighter_gobj->gobjproc_head != NULL) &&
        (fighter_gobj->gobjproc_head->exec.func == gcPlayCamAnim))
    {
        gNdsOpeningRoomScene1CameraProcessSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_PROCESS_READY;
    }
    if ((main_cobj != NULL) &&
        (main_cobj->camanim_joint.event32 != NULL) &&
        (fighter_cobj != NULL) &&
        (fighter_cobj->camanim_joint.event32 != NULL))
    {
        gNdsOpeningRoomScene1CameraAnimSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_CAMANIM_READY;
    }
    if (ndsOpeningRoomCObjViewportIsSet(main_cobj) &&
        ndsOpeningRoomCObjViewportIsSet(fighter_cobj) &&
        (main_cobj->projection.persp.near == 80.0F) &&
        (main_cobj->projection.persp.far == 15000.0F) &&
        (fighter_cobj->projection.persp.near == 80.0F) &&
        (fighter_cobj->projection.persp.far == 15000.0F))
    {
        gNdsOpeningRoomScene1CameraViewportSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_VIEWPORT_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        ((main_cobj->flags & COBJ_FLAG_DLBUFFERS) != 0) &&
        ((fighter_cobj->flags & COBJ_FLAG_DLBUFFERS) != 0))
    {
        gNdsOpeningRoomScene1CameraDLBufferSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_DLBUFFER_READY;
    }

    gNdsOpeningRoomScene1CameraCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_READY_MASK)
    {
        gNdsOpeningRoomScene1CameraCreateResult =
            NDS_OPENING_ROOM_SCENE1_CAMERA_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureScene2CameraEjectBefore(void)
{
    if (sNdsOpeningRoomScene2CameraEjectCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomScene2EjectMainCameraGObj = sMVOpeningRoomMainCameraGObj;
    sNdsOpeningRoomScene2EjectFighterCameraGObj = sMVOpeningRoomFighterCameraGObj;
    gNdsOpeningRoomScene2CameraEjectBeforeGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomScene2CameraEjectBeforeCameraCount = sGCCamerasActiveNum;
    sNdsOpeningRoomScene2CameraEjectCaptured = 1;
}

void ndsOpeningRoomRecordScene2CameraEject(void)
{
    GObj *main_gobj = sNdsOpeningRoomScene2EjectMainCameraGObj;
    GObj *fighter_gobj = sNdsOpeningRoomScene2EjectFighterCameraGObj;
    u32 mask = 0;

    if ((main_gobj == NULL) || (fighter_gobj == NULL) ||
        (sNdsOpeningRoomScene2CameraEjectCaptured == 0))
    {
        return;
    }

    gNdsOpeningRoomScene2CameraEjectAfterGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomScene2CameraEjectAfterCameraCount = sGCCamerasActiveNum;

    if (!ndsOpeningRoomContainsCommonGObj(main_gobj) &&
        (main_gobj->obj_kind == nGCCommonAppendNone) &&
        (main_gobj->obj == NULL))
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_MAIN_UNLINKED;
    }
    if (!ndsOpeningRoomContainsCommonGObj(fighter_gobj) &&
        (fighter_gobj->obj_kind == nGCCommonAppendNone) &&
        (fighter_gobj->obj == NULL))
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_FIGHTER_UNLINKED;
    }
    if (gNdsOpeningRoomScene2CameraEjectBeforeCameraCount ==
        (gNdsOpeningRoomScene2CameraEjectAfterCameraCount + 2u))
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_COBJ_FREED;
    }

    gNdsOpeningRoomScene2CameraEjectMask = mask;
    if ((mask & NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_READY_MASK) ==
        NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_READY_MASK)
    {
        gNdsOpeningRoomScene2CameraEjectResult =
            NDS_OPENING_ROOM_SCENE2_CAMERA_EJECT_PASS;
    }
}

void ndsOpeningRoomCaptureScene2CameraCountsBefore(void)
{
    if (sNdsOpeningRoomScene2CameraCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomScene2CameraGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomScene2CameraCObjsBefore = sGCCamerasActiveNum;
    sNdsOpeningRoomScene2CameraXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomScene2CameraAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomScene2CameraCountsCaptured = 1;
}

void ndsOpeningRoomCaptureScene2CameraCreation(void)
{
    GObj *main_gobj = sMVOpeningRoomMainCameraGObj;
    GObj *fighter_gobj = sMVOpeningRoomFighterCameraGObj;
    CObj *main_cobj;
    CObj *fighter_cobj;
    u32 mask = 0;

    if ((main_gobj == NULL) || (fighter_gobj == NULL) ||
        (sNdsOpeningRoomScene2CameraCountsCaptured == 0))
    {
        return;
    }

    main_cobj = CObjGetStruct(main_gobj);
    fighter_cobj = CObjGetStruct(fighter_gobj);
    gNdsOpeningRoomScene2CameraCreateGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomScene2CameraGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomScene2CameraGObjsBefore;
    gNdsOpeningRoomScene2CameraCObjDelta =
        sGCCamerasActiveNum - sNdsOpeningRoomScene2CameraCObjsBefore;
    gNdsOpeningRoomScene2CameraXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomScene2CameraXObjsBefore;
    gNdsOpeningRoomScene2CameraAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomScene2CameraAObjsBefore;

    if (gNdsOpeningRoomScene2CameraGObjDelta == 2u)
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_MAIN_GOBJ_READY;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_FIGHTER_GOBJ_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        (gNdsOpeningRoomScene2CameraCObjDelta == 2u))
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_COBJ_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        (gNdsOpeningRoomScene2CameraXObjDelta == 4u) &&
        (main_cobj->xobjs_num == 2) &&
        (fighter_cobj->xobjs_num == 2))
    {
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_XOBJ_READY;
    }
    if ((main_gobj->proc_display == func_80017EC0) &&
        (main_gobj->dl_link_priority == 80) &&
        (main_gobj->camera_mask == (1ULL << 6)) &&
        (fighter_gobj->proc_display == func_80017EC0) &&
        (fighter_gobj->dl_link_priority == 40) &&
        (fighter_gobj->camera_mask == ((1ULL << 27) | (1ULL << 9))))
    {
        gNdsOpeningRoomScene2CameraDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_DISPLAY_READY;
    }
    if ((main_gobj->gobjproc_head != NULL) &&
        (main_gobj->gobjproc_head->exec.func == gcPlayCamAnim) &&
        (fighter_gobj->gobjproc_head != NULL) &&
        (fighter_gobj->gobjproc_head->exec.func == gcPlayCamAnim))
    {
        gNdsOpeningRoomScene2CameraProcessSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_PROCESS_READY;
    }
    if ((main_cobj != NULL) &&
        (main_cobj->camanim_joint.event32 != NULL) &&
        (fighter_cobj != NULL) &&
        (fighter_cobj->camanim_joint.event32 != NULL))
    {
        gNdsOpeningRoomScene2CameraAnimSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_CAMANIM_READY;
    }
    if (ndsOpeningRoomCObjViewportIsSet(main_cobj) &&
        ndsOpeningRoomCObjViewportIsSet(fighter_cobj))
    {
        gNdsOpeningRoomScene2CameraViewportSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_VIEWPORT_READY;
    }
    if ((main_cobj != NULL) && (fighter_cobj != NULL) &&
        ((main_cobj->flags & COBJ_FLAG_DLBUFFERS) != 0) &&
        ((fighter_cobj->flags & COBJ_FLAG_DLBUFFERS) != 0))
    {
        gNdsOpeningRoomScene2CameraDLBufferSet = 1;
        mask |= NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_DLBUFFER_READY;
    }

    gNdsOpeningRoomScene2CameraCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_READY_MASK)
    {
        gNdsOpeningRoomScene2CameraCreateResult =
            NDS_OPENING_ROOM_SCENE2_CAMERA_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureCloseUpOverlayCameraCountsBefore(void)
{
    if (sNdsOpeningRoomCloseUpOverlayCameraCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomCloseUpOverlayCameraGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomCloseUpOverlayCameraCObjsBefore = sGCCamerasActiveNum;
    sNdsOpeningRoomCloseUpOverlayCameraXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomCloseUpOverlayCameraCountsCaptured = 1;
}

void ndsOpeningRoomCaptureCloseUpOverlayCameraCreation(void *gobj)
{
    GObj *camera_gobj = (GObj*)gobj;
    CObj *cobj;
    u32 mask = 0;

    if ((camera_gobj == NULL) ||
        (sNdsOpeningRoomCloseUpOverlayCameraCountsCaptured == 0))
    {
        return;
    }

    cobj = CObjGetStruct(camera_gobj);
    gNdsOpeningRoomCloseUpOverlayCameraCreateGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomCloseUpOverlayCameraGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomCloseUpOverlayCameraGObjsBefore;
    gNdsOpeningRoomCloseUpOverlayCameraCObjDelta =
        sGCCamerasActiveNum - sNdsOpeningRoomCloseUpOverlayCameraCObjsBefore;
    gNdsOpeningRoomCloseUpOverlayCameraXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomCloseUpOverlayCameraXObjsBefore;

    if (gNdsOpeningRoomCloseUpOverlayCameraGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_GOBJ_READY;
    }
    if ((cobj != NULL) &&
        (gNdsOpeningRoomCloseUpOverlayCameraCObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_COBJ_READY;
    }
    if (gNdsOpeningRoomCloseUpOverlayCameraXObjDelta == 0u)
    {
        mask |= NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_XOBJ_READY;
    }
    if ((camera_gobj->proc_display == lbCommonDrawSprite) &&
        (camera_gobj->dl_link_priority == 60) &&
        (camera_gobj->camera_mask == (1ULL << 26)))
    {
        gNdsOpeningRoomCloseUpOverlayCameraDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_DISPLAY_READY;
    }
    if (ndsOpeningRoomCObjViewportIsSet(cobj))
    {
        gNdsOpeningRoomCloseUpOverlayCameraViewportSet = 1;
        mask |= NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_VIEWPORT_READY;
    }

    gNdsOpeningRoomCloseUpOverlayCameraCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_READY_MASK)
    {
        gNdsOpeningRoomCloseUpOverlayCameraCreateResult =
            NDS_OPENING_ROOM_CLOSEUP_CAMERA_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureWallpaperCameraCountsBefore(void)
{
    if (sNdsOpeningRoomWallpaperCameraCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomWallpaperCameraGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomWallpaperCameraCObjsBefore = sGCCamerasActiveNum;
    sNdsOpeningRoomWallpaperCameraXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomWallpaperCameraCountsCaptured = 1;
}

void ndsOpeningRoomCaptureWallpaperCameraCreation(void *gobj)
{
    GObj *camera_gobj = (GObj*)gobj;
    CObj *cobj;
    u32 mask = 0;

    if ((camera_gobj == NULL) ||
        (sNdsOpeningRoomWallpaperCameraCountsCaptured == 0))
    {
        return;
    }

    cobj = CObjGetStruct(camera_gobj);
    gNdsOpeningRoomWallpaperCameraCreateGObjCount =
        (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomWallpaperCameraGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomWallpaperCameraGObjsBefore;
    gNdsOpeningRoomWallpaperCameraCObjDelta =
        sGCCamerasActiveNum - sNdsOpeningRoomWallpaperCameraCObjsBefore;
    gNdsOpeningRoomWallpaperCameraXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomWallpaperCameraXObjsBefore;

    if (gNdsOpeningRoomWallpaperCameraGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_GOBJ_READY;
    }
    if ((cobj != NULL) && (gNdsOpeningRoomWallpaperCameraCObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_COBJ_READY;
    }
    if (gNdsOpeningRoomWallpaperCameraXObjDelta == 0u)
    {
        mask |= NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_XOBJ_READY;
    }
    if ((camera_gobj->proc_display == lbCommonDrawSprite) &&
        (camera_gobj->dl_link_priority == 90) &&
        (camera_gobj->camera_mask == (1ULL << 28)))
    {
        gNdsOpeningRoomWallpaperCameraDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_DISPLAY_READY;
    }
    if ((cobj != NULL) &&
        (cobj->viewport.vp.vscale[0] == 600) &&
        (cobj->viewport.vp.vscale[1] == 440) &&
        (cobj->viewport.vp.vtrans[0] == 640) &&
        (cobj->viewport.vp.vtrans[1] == 480) &&
        (cobj->viewport.vp.vscale[2] == 511) &&
        (cobj->viewport.vp.vtrans[2] == 511))
    {
        gNdsOpeningRoomWallpaperCameraViewportSet = 1;
        mask |= NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_VIEWPORT_READY;
    }

    gNdsOpeningRoomWallpaperCameraCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_READY_MASK)
    {
        gNdsOpeningRoomWallpaperCameraCreateResult =
            NDS_OPENING_ROOM_WALLPAPER_CAMERA_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureLogoCameraCountsBefore(void)
{
    if (sNdsOpeningRoomLogoCameraCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomLogoCameraGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomLogoCameraCObjsBefore = sGCCamerasActiveNum;
    sNdsOpeningRoomLogoCameraXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomLogoCameraAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomLogoCameraCountsCaptured = 1;
}

void ndsOpeningRoomCaptureLogoCameraCreation(void *gobj)
{
    GObj *camera_gobj = (GObj*)gobj;
    CObj *cobj;
    u32 mask = 0;

    if ((camera_gobj == NULL) ||
        (sNdsOpeningRoomLogoCameraCountsCaptured == 0))
    {
        return;
    }

    cobj = CObjGetStruct(camera_gobj);
    gNdsOpeningRoomLogoCameraCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomLogoCameraGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomLogoCameraGObjsBefore;
    gNdsOpeningRoomLogoCameraCObjDelta =
        sGCCamerasActiveNum - sNdsOpeningRoomLogoCameraCObjsBefore;
    gNdsOpeningRoomLogoCameraXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomLogoCameraXObjsBefore;
    gNdsOpeningRoomLogoCameraAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomLogoCameraAObjsBefore;

    if (gNdsOpeningRoomLogoCameraGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_GOBJ_READY;
    }
    if ((cobj != NULL) && (gNdsOpeningRoomLogoCameraCObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_COBJ_READY;
    }
    if (gNdsOpeningRoomLogoCameraXObjDelta == 2u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_XOBJ_READY;
    }
    if ((camera_gobj->proc_display == func_80017EC0) &&
        (camera_gobj->dl_link_priority == 50) &&
        (camera_gobj->camera_mask == (1ULL << 29)))
    {
        gNdsOpeningRoomLogoCameraDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_DISPLAY_READY;
    }
    if ((camera_gobj->gobjproc_head != NULL) &&
        (camera_gobj->gobjproc_head->exec.func == gcPlayCamAnim))
    {
        gNdsOpeningRoomLogoCameraProcessSet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_PROCESS_READY;
    }
    if ((cobj != NULL) && (cobj->camanim_joint.event32 != NULL))
    {
        gNdsOpeningRoomLogoCameraAnimSet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_CAMANIM_READY;
    }
    if ((cobj != NULL) &&
        (cobj->viewport.vp.vscale[0] == 600) &&
        (cobj->viewport.vp.vscale[1] == 440) &&
        (cobj->viewport.vp.vtrans[0] == 640) &&
        (cobj->viewport.vp.vtrans[1] == 480) &&
        (cobj->viewport.vp.vscale[2] == 511) &&
        (cobj->viewport.vp.vtrans[2] == 511))
    {
        gNdsOpeningRoomLogoCameraViewportSet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_VIEWPORT_READY;
    }

    gNdsOpeningRoomLogoCameraCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_READY_MASK)
    {
        gNdsOpeningRoomLogoCameraCreateResult =
            NDS_OPENING_ROOM_LOGO_CAMERA_CREATE_PASS;
    }
}

void ndsOpeningRoomCaptureLogoCountsBefore(void)
{
    if (sNdsOpeningRoomLogoCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomLogoGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomLogoDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomLogoXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomLogoMObjsBefore = sGCMaterialsActive;
    sNdsOpeningRoomLogoAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomLogoCountsCaptured = 1;
}

void ndsOpeningRoomCaptureLogoCreation(void *gobj)
{
    GObj *logo_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;
    u32 dobj_count = 0;
    u32 mobj_count = 0;
    u32 matanim_count = 0;

    if ((logo_gobj == NULL) || (sNdsOpeningRoomLogoCountsCaptured == 0))
    {
        return;
    }

    gNdsOpeningRoomLogoCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomLogoGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomLogoGObjsBefore;
    gNdsOpeningRoomLogoDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomLogoDObjsBefore;
    gNdsOpeningRoomLogoXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomLogoXObjsBefore;
    gNdsOpeningRoomLogoMObjDelta =
        sGCMaterialsActive - sNdsOpeningRoomLogoMObjsBefore;
    gNdsOpeningRoomLogoAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomLogoAObjsBefore;

    if (gNdsOpeningRoomLogoGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_GOBJ_READY;
    }
    if (gNdsOpeningRoomLogoDObjDelta == 2u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomLogoXObjDelta == 4u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_XOBJ_READY;
    }
    if (gNdsOpeningRoomLogoMObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_MOBJ_READY;
    }
    if ((logo_gobj->proc_display == gcDrawDObjTreeDLLinksForGObj) &&
        (logo_gobj->dl_link_id == 29))
    {
        gNdsOpeningRoomLogoDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_DISPLAY_READY;
    }

    dobj = DObjGetStruct(logo_gobj);
    while (dobj != NULL)
    {
        dobj_count++;
        if (dobj->mobj != NULL)
        {
            mobj_count++;
            if (dobj->mobj->matanim_joint.event32 != NULL)
            {
                matanim_count++;
            }
        }
        dobj = gcGetTreeDObjNext(dobj);
    }
    if ((dobj_count == 2u) && (mobj_count == 1u))
    {
        gNdsOpeningRoomLogoMObjSet = 1;
    }
    if (matanim_count == 1u)
    {
        gNdsOpeningRoomLogoMatAnimSet = 1;
        mask |= NDS_OPENING_ROOM_LOGO_CREATE_MATANIM_READY;
    }

    gNdsOpeningRoomLogoCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_LOGO_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_LOGO_CREATE_READY_MASK)
    {
        gNdsOpeningRoomLogoCreateResult = NDS_OPENING_ROOM_LOGO_CREATE_PASS;
    }
}

void ndsOpeningRoomRecordLogoEject(void *gobj)
{
    GObj *logo_gobj = (GObj*)gobj;
    u32 mask = 0;

    if (logo_gobj == NULL)
    {
        gNdsOpeningRoomLogoEjectUnlinkedMask = 0;
        return;
    }

    if (!ndsOpeningRoomContainsCommonGObj(logo_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_COMMON_UNLINKED;
    }
    if (!ndsOpeningRoomContainsDLGObj(logo_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_DL_UNLINKED;
    }

    gNdsOpeningRoomLogoEjectUnlinkedMask = mask;
    if ((mask & NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK) ==
        NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK)
    {
        gNdsOpeningRoomLogoEjectResult = NDS_OPENING_ROOM_LOGO_EJECT_PASS;
    }
}

void ndsOpeningRoomCaptureBossShadowCountsBefore(void)
{
    if (sNdsOpeningRoomBossShadowCountsCaptured != 0)
    {
        return;
    }

    sNdsOpeningRoomBossShadowGObjsBefore = sGCCommonsActiveNum;
    sNdsOpeningRoomBossShadowDObjsBefore = sGCDrawsActiveNum;
    sNdsOpeningRoomBossShadowXObjsBefore = sGCMatrixesActiveNum;
    sNdsOpeningRoomBossShadowAObjsBefore = sGCAnimsActiveNum;
    sNdsOpeningRoomBossShadowCountsCaptured = 1;
}

void ndsOpeningRoomCaptureBossShadowCreation(void *gobj)
{
    GObj *boss_shadow_gobj = (GObj*)gobj;
    DObj *dobj;
    u32 mask = 0;

    if ((boss_shadow_gobj == NULL) ||
        (sNdsOpeningRoomBossShadowCountsCaptured == 0))
    {
        return;
    }

    dobj = DObjGetStruct(boss_shadow_gobj);
    gNdsOpeningRoomBossShadowCreateGObjCount = (u32)gcGetGObjsActiveNum();
    gNdsOpeningRoomBossShadowGObjDelta =
        sGCCommonsActiveNum - sNdsOpeningRoomBossShadowGObjsBefore;
    gNdsOpeningRoomBossShadowDObjDelta =
        sGCDrawsActiveNum - sNdsOpeningRoomBossShadowDObjsBefore;
    gNdsOpeningRoomBossShadowXObjDelta =
        sGCMatrixesActiveNum - sNdsOpeningRoomBossShadowXObjsBefore;
    gNdsOpeningRoomBossShadowAObjDelta =
        sGCAnimsActiveNum - sNdsOpeningRoomBossShadowAObjsBefore;

    if (gNdsOpeningRoomBossShadowGObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_GOBJ_READY;
    }
    if ((dobj != NULL) && (gNdsOpeningRoomBossShadowDObjDelta == 1u))
    {
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_DOBJ_READY;
    }
    if (gNdsOpeningRoomBossShadowXObjDelta == 1u)
    {
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_XOBJ_READY;
    }
    if ((boss_shadow_gobj->gobjproc_head != NULL) &&
        (boss_shadow_gobj->gobjproc_head->exec.func == gcPlayAnimAll))
    {
        gNdsOpeningRoomBossShadowProcessSet = 1;
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_PROCESS_READY;
    }
    if ((boss_shadow_gobj->proc_display == gcDrawDObjDLHead1) &&
        (boss_shadow_gobj->dl_link_id == 9))
    {
        gNdsOpeningRoomBossShadowDisplaySet = 1;
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_DISPLAY_READY;
    }
    if ((dobj != NULL) && (dobj->anim_joint.event32 != NULL))
    {
        gNdsOpeningRoomBossShadowAnimSet = 1;
        mask |= NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_ANIM_READY;
    }

    gNdsOpeningRoomBossShadowCreateMask = mask;
    if ((mask & NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_READY_MASK) ==
        NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_READY_MASK)
    {
        gNdsOpeningRoomBossShadowCreateResult =
            NDS_OPENING_ROOM_BOSS_SHADOW_CREATE_PASS;
    }
}

void ndsOpeningRoomRecordBossShadowEject(void *gobj)
{
    GObj *boss_shadow_gobj = (GObj*)gobj;
    u32 mask = 0;

    if (boss_shadow_gobj == NULL)
    {
        gNdsOpeningRoomBossShadowEjectUnlinkedMask = 0;
        return;
    }

    if (!ndsOpeningRoomContainsCommonGObj(boss_shadow_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_COMMON_UNLINKED;
    }
    if (!ndsOpeningRoomContainsDLGObj(boss_shadow_gobj))
    {
        mask |= NDS_OPENING_ROOM_OVERLAY_EJECT_DL_UNLINKED;
    }

    gNdsOpeningRoomBossShadowEjectUnlinkedMask = mask;
    if ((mask & NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK) ==
        NDS_OPENING_ROOM_OVERLAY_EJECT_UNLINKED_MASK)
    {
        gNdsOpeningRoomBossShadowEjectResult =
            NDS_OPENING_ROOM_BOSS_SHADOW_EJECT_PASS;
    }
}

static void ndsDrainTaskmanQueue(OSMesgQueue *queue)
{
    while (osRecvMesg(queue, NULL, OS_MESG_NOBLOCK) != -1)
    {
    }
}

static void ndsPrepareTaskmanRun(void)
{
    D_800454BC = 0;
    ndsDrainTaskmanQueue(&sSYTaskmanContextMesgQueue);
    ndsDrainTaskmanQueue(&sSYTaskmanResetMesgQueue);
    ndsDrainTaskmanQueue(&sSYTaskmanGameTicMesgQueue);
    sSYTaskmanStatus = nSYTaskmanStatusDefault;
    sSYTaskmanFramebufferID = -1;
    gSYTaskmanTaskID = 1;
    gSYSchedulerIsCustomFramebuffer = FALSE;
    D_80046638[0] = 0;
    D_80046638[1] = 0;
}

static void ndsFinishTaskmanRun(void)
{
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
    if (gSCManagerSceneData.scene_curr == nSCKindVSBattle)
    {
        syAudioStopBGMAll();
    }
#endif
    func_80005BFC();
    ndsDrainTaskmanQueue(&sSYTaskmanContextMesgQueue);
    ndsDrainTaskmanQueue(&sSYTaskmanResetMesgQueue);
    ndsDrainTaskmanQueue(&sSYTaskmanGameTicMesgQueue);
    syRdpSetFuncLights(NULL);
    D_800454BC = 2;

    gNdsTaskmanCleanupQueuesEmpty =
        (sSYTaskmanContextMesgQueue.validCount == 0) &&
        (sSYTaskmanResetMesgQueue.validCount == 0) &&
        (sSYTaskmanGameTicMesgQueue.validCount == 0);
    gNdsTaskmanCleanupMode = D_800454BC;
    gNdsTaskmanCleanupResult = NDS_TASKMAN_CLEANUP_PASS;
    gNdsTaskmanReturnCount++;
}

static void ndsOpeningMoviePresentFrame(void)
{
    (void)ndsPlatformReadInput();
    ndsPlatformBeginFrame();
    ndsPlatformRenderDebugHud();
    ndsPlatformEndFrame();
    gNdsFrameCounter++;
    gNdsOpeningMoviePresentFrameCount++;
}

static void ndsOpeningRoomRenderSelectedDLPreview(void);

static ub8 ndsOpeningRoomShouldRunDrawProbe(u32 tick)
{
    if (gNdsOpeningRoomDrawProbeCount == 0)
    {
        return TRUE;
    }
    if (tick >= NDS_OPENING_ROOM_HANDOFF_TICK)
    {
        return TRUE;
    }
    if (gNdsOpeningRoomDLPreviewResult != NDS_OPENING_ROOM_DL_PREVIEW_PASS)
    {
        return TRUE;
    }
    return FALSE;
}

static void ndsRunBoundedOpeningRoomDraw(struct SYTaskFunction *tfunc)
{
    gNdsOpeningRoomDrawTickCount = (u32)sMVOpeningRoomTotalTimeTics;
    gNdsOpeningRoomDrawFrameCount = dSYTaskmanFrameCount;
    gNdsOpeningRoomDrawProbeCount++;

    if ((tfunc == NULL) || (tfunc->scene_draw == NULL))
    {
        gNdsOpeningRoomDrawBlocker =
            NDS_OPENING_ROOM_DRAW_BLOCKER_NO_SCENE_DRAW;
        return;
    }

    gNdsOpeningRoomDrawResult = NDS_OPENING_ROOM_DRAW_PASS;
    tfunc->scene_draw();
    ndsOpeningRoomRenderSelectedDLPreview();
    dSYTaskmanFrameCount++;
    ndsOpeningMoviePresentFrame();
}

static void ndsReuseBoundedOpeningRoomPreview(void)
{
    gNdsOpeningRoomDrawReuseCount++;
    ndsOpeningMoviePresentFrame();
}

void ndsOpeningMovieRecordRoomHandoff(u32 tick, u32 next_scene)
{
    gNdsOpeningMovieRoomHandoffTick = tick;
    gNdsOpeningMovieRoomHandoffScene = next_scene;
    gNdsOpeningMovieRoomHandoffResult =
        NDS_OPENING_MOVIE_ROOM_HANDOFF_PASS;
}

void ndsOpeningPortraitsRecordStart(void)
{
    gNdsOpeningPortraitsDispatchCount++;
    gNdsOpeningPortraitsStartResult = NDS_OPENING_PORTRAITS_START_PASS;
}

void ndsOpeningPortraitsRecordFuncStart(void)
{
    gNdsOpeningPortraitsFuncStartResult =
        NDS_OPENING_PORTRAITS_FUNC_START_PASS;
}

void ndsOpeningPortraitsRecordRunTick(void)
{
    gNdsOpeningPortraitsTickCount = (u32)sMVOpeningPortraitsTotalTimeTics;
    if (sMVOpeningPortraitsTotalTimeTics > 0)
    {
        gNdsOpeningPortraitsUpdateResult =
            NDS_OPENING_PORTRAITS_UPDATE_PASS;
    }
    if ((sSYTaskmanStatus == nSYTaskmanStatusLoadScene) &&
        (gSCManagerSceneData.scene_curr != nSCKindOpeningPortraits))
    {
        gNdsOpeningPortraitsNextSceneKind = gSCManagerSceneData.scene_curr;
        gNdsOpeningPortraitsNextSceneResult =
            NDS_OPENING_PORTRAITS_NEXT_SCENE_PASS;
    }
}

void ndsOpeningMarioRecordStart(void)
{
    gNdsOpeningMarioDispatchCount++;
    gNdsOpeningMarioStartResult = NDS_OPENING_MARIO_START_PASS;
}

void ndsOpeningMarioRecordFuncStart(void)
{
    gNdsOpeningMarioFuncStartResult =
        NDS_OPENING_MARIO_FUNC_START_PASS;
}

void ndsOpeningMarioRecordFighterDeferred(void)
{
    gNdsOpeningMarioFighterDeferredTick =
        (u32)sMVOpeningMarioTotalTimeTics;
    gNdsOpeningMarioFighterDeferredResult =
        NDS_OPENING_MARIO_FIGHTER_DEFER_PASS;
}

void ndsOpeningMarioRecordRunTick(void)
{
    gNdsOpeningMarioTickCount = (u32)sMVOpeningMarioTotalTimeTics;
    if (sMVOpeningMarioTotalTimeTics > 0)
    {
        gNdsOpeningMarioUpdateResult = NDS_OPENING_MARIO_UPDATE_PASS;
    }
    if ((sSYTaskmanStatus == nSYTaskmanStatusLoadScene) &&
        (gSCManagerSceneData.scene_curr != nSCKindOpeningMario))
    {
        gNdsOpeningMarioNextSceneKind = gSCManagerSceneData.scene_curr;
        gNdsOpeningMarioNextSceneResult =
            NDS_OPENING_MARIO_NEXT_SCENE_PASS;
    }
}

static u32 ndsOpeningNameSceneMask(u32 scene_kind)
{
    if ((scene_kind < nSCKindOpeningMario) ||
        (scene_kind > nSCKindOpeningKirby))
    {
        return 0;
    }
    return 1u << (scene_kind - nSCKindOpeningMario);
}

static sb32 ndsOpeningIsImportedNameScene(u32 scene_kind)
{
    return (ndsOpeningNameSceneMask(scene_kind) != 0) ? TRUE : FALSE;
}

void ndsOpeningNameRecordStart(u32 scene_kind)
{
    u32 mask = ndsOpeningNameSceneMask(scene_kind);

    gNdsOpeningNameSceneDispatchCount++;
    gNdsOpeningNameSceneLastKind = scene_kind;
    gNdsOpeningNameSceneDispatchMask |= mask;
}

void ndsOpeningNameRecordFuncStart(u32 scene_kind)
{
    gNdsOpeningNameSceneLastKind = scene_kind;
    gNdsOpeningNameSceneFuncStartMask |= ndsOpeningNameSceneMask(scene_kind);
}

void ndsOpeningNameRecordFighterDeferred(u32 scene_kind, u32 tick)
{
    gNdsOpeningNameSceneLastKind = scene_kind;
    gNdsOpeningNameSceneLastTick = tick;
    gNdsOpeningNameSceneFighterDeferMask |=
        ndsOpeningNameSceneMask(scene_kind);
}

void ndsOpeningNameRecordRunTick(u32 scene_kind, u32 tick)
{
    gNdsOpeningNameSceneLastKind = scene_kind;
    gNdsOpeningNameSceneLastTick = tick;
    if (tick > 0)
    {
        gNdsOpeningNameSceneUpdateMask |= ndsOpeningNameSceneMask(scene_kind);
    }
    if ((sSYTaskmanStatus == nSYTaskmanStatusLoadScene) &&
        (gSCManagerSceneData.scene_curr != scene_kind))
    {
        gNdsOpeningNameSceneLastNextKind = gSCManagerSceneData.scene_curr;
        gNdsOpeningNameSceneNextMask |= ndsOpeningNameSceneMask(scene_kind);
    }
}

static void ndsTitleRenderPreview(void);

static void ndsRunBoundedOpeningPortraitsDraw(struct SYTaskFunction *tfunc)
{
    if ((tfunc == NULL) || (tfunc->scene_draw == NULL))
    {
        return;
    }

    tfunc->scene_draw();
    dSYTaskmanFrameCount++;
    ndsOpeningMoviePresentFrame();
}

/* DS seam: bound the original task loop and one startup draw.
 *
 * syTaskmanLoadScene calls syTaskmanRunTask(&sSYTaskmanDefaultFunction) at its
 * end (sys/taskman.c). The real RunTask would enter the per-frame
 * gcRunAll/gcDrawAll loop, which needs the threading scheduler, display-list
 * pipeline, and RSP/RDP backend that are not yet imported. This DS
 * implementation instead records that the original flow reached the loop entry,
 * snapshots the real startup state, runs a bounded number of original task
 * updates, invokes one startup scene_draw while the original N64 logo SObj is
 * still alive, mirrors the original cleanup tail, and returns to the scene
 * manager. This preserves the original control flow while keeping the draw path
 * bounded to the first imported sprite asset. */
void syTaskmanRunTask(struct SYTaskFunction *tfunc)
{
    ndsPrepareTaskmanRun();

#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
        /* tic 120 starts a finite winner sequence; run long enough for the
         * original Results audio thread to observe AL_STOPPED and start BGM 22. */
        const u32 fast_update_max = NDS_AUDIO_BGM_RESULTS_FAST_UPDATE_MAX;
#else
        const u32 fast_update_max = 132u;
#endif
        u32 updates = 0;

        ndsPlatformClearBattleTextHud();
        ndsPlatformSetOriginalSpriteOverlayEnabled(TRUE);
        while ((tfunc != NULL) && (tfunc->task_update != NULL) &&
               (sSYTaskmanStatus != nSYTaskmanStatusLoadScene) &&
               ((NDS_HARNESS_FAST_LOGIC == 0) ||
                (updates < fast_update_max)))
        {
            if (NDS_HARNESS_FAST_LOGIC == 0)
            {
                (void)ndsPlatformReadInput();
                syControllerReadDeviceData();
                syControllerUpdateGlobalData();
            }
            tfunc->task_update(tfunc);
            ndsAudioBackendUpdate();
            dSYTaskmanUpdateCount++;
            updates++;
            ndsMNVSResultsRecordFrame();

            /* Results owns fade progression in display callbacks; preserve the
             * source one-update/one-draw contract in fast verification too. */
            {
                gNdsRendererProfileFrameCount++;
                /* taskman.c:1093-1100 resets these arenas before every
                 * source scene draw. */
                syTaskmanResetGraphicsHeap();
                func_80004AB0();
                ndsSObjPreviewBeginFrame();
                if (tfunc->scene_draw != NULL)
                {
                    tfunc->scene_draw();
                    dSYTaskmanFrameCount++;
                }
                ndsSObjPreviewEndFrame();
                ndsPlatformEndFrame();
            }
        }

        ndsFinishTaskmanRun();
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
        {
            ndsPlatformSetOriginalSpriteOverlayEnabled(FALSE);
            return;
        }
        osStopThread(NULL);
        return;
    }
#endif

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningRoom)
    {
        gNdsOpeningRoomDL0Size = sizeof(Gfx) * 1500u;
        gNdsOpeningRoomDL1Size = sizeof(Gfx) * 512u;
        gNdsOpeningRoomGraphicsHeapSize = 0x8000u;
        gNdsOpeningRoomRdpBufferSize = 0xC000u;
        gNdsOpeningRoomMallocCount =
            gNdsTaskmanMallocCount - gNdsStartupTaskmanMallocCount;
        gNdsOpeningRoomGObjCount = sGCCommonsActiveNum;
        gNdsOpeningRoomCameraCount = sGCCamerasActiveNum;

        while ((tfunc != NULL) && (tfunc->task_update != NULL) &&
               ((u32)sMVOpeningRoomTotalTimeTics <
                NDS_OPENING_ROOM_HANDOFF_TICK))
        {
            if ((u32)sMVOpeningRoomTotalTimeTics ==
                NDS_OPENING_ROOM_PRE_ASSET_TICK)
            {
                gNdsOpeningRoomPreAssetResult =
                    NDS_OPENING_ROOM_PRE_ASSET_PASS;
            }

            tfunc->task_update(tfunc);
            dSYTaskmanUpdateCount++;

            gNdsOpeningRoomTickCount = (u32)sMVOpeningRoomTotalTimeTics;
            if (sMVOpeningRoomTotalTimeTics > 0)
            {
                gNdsOpeningRoomUpdateResult = NDS_OPENING_ROOM_UPDATE_PASS;
            }
            if ((sSYTaskmanStatus != nSYTaskmanStatusLoadScene) &&
                ((u32)sMVOpeningRoomTotalTimeTics > 0u) &&
                ((u32)sMVOpeningRoomTotalTimeTics <
                 NDS_OPENING_ROOM_TICK560_RUN_TICK) &&
                (((u32)sMVOpeningRoomTotalTimeTics %
                  NDS_OPENING_MOVIE_DRAW_INTERVAL) == 0u))
            {
                ndsOpeningMoviePresentFrame();
            }
            if (((u32)sMVOpeningRoomTotalTimeTics >=
                 NDS_OPENING_ROOM_TICK560_RUN_TICK) &&
                ((((u32)sMVOpeningRoomTotalTimeTics %
                   NDS_OPENING_MOVIE_DRAW_INTERVAL) == 0u) ||
                 ((u32)sMVOpeningRoomTotalTimeTics ==
                  NDS_OPENING_ROOM_HANDOFF_TICK)))
            {
                if (ndsOpeningRoomShouldRunDrawProbe(
                        (u32)sMVOpeningRoomTotalTimeTics) != FALSE)
                {
                    ndsRunBoundedOpeningRoomDraw(tfunc);
                }
                else
                {
                    ndsReuseBoundedOpeningRoomPreview();
                }
            }
            if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
            {
                ndsFinishTaskmanRun();
                return;
            }
        }

        ndsRunBoundedOpeningRoomDraw(tfunc);

        gNdsOpeningRoomTickCount = (u32)sMVOpeningRoomTotalTimeTics;
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningPortraits)
    {
        while ((tfunc != NULL) && (tfunc->task_update != NULL) &&
               ((u32)sMVOpeningPortraitsTotalTimeTics <
                NDS_OPENING_PORTRAITS_HANDOFF_TICK))
        {
            tfunc->task_update(tfunc);
            dSYTaskmanUpdateCount++;
            ndsOpeningPortraitsRecordRunTick();
            if ((sSYTaskmanStatus == nSYTaskmanStatusLoadScene) &&
                (gSCManagerSceneData.scene_curr != nSCKindOpeningPortraits))
            {
                ndsFinishTaskmanRun();
                return;
            }
            if ((((u32)sMVOpeningPortraitsTotalTimeTics %
                  NDS_OPENING_MOVIE_DRAW_INTERVAL) == 0u) ||
                ((u32)sMVOpeningPortraitsTotalTimeTics ==
                 NDS_OPENING_PORTRAITS_HANDOFF_TICK))
            {
                ndsRunBoundedOpeningPortraitsDraw(tfunc);
            }
            if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
            {
                ndsOpeningPortraitsRecordRunTick();
                ndsFinishTaskmanRun();
                return;
            }
        }

        ndsRunBoundedOpeningPortraitsDraw(tfunc);
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (ndsOpeningIsImportedNameScene(gSCManagerSceneData.scene_curr) != FALSE)
    {
        u32 guard = 0;

        while ((tfunc != NULL) && (tfunc->task_update != NULL) &&
               (guard < (NDS_OPENING_MARIO_HANDOFF_TICK + 4u)))
        {
            tfunc->task_update(tfunc);
            guard++;
            dSYTaskmanUpdateCount++;
            if ((guard == 10u) ||
                ((guard % NDS_OPENING_MOVIE_DRAW_INTERVAL) == 0u) ||
                (guard == NDS_OPENING_MARIO_HANDOFF_TICK))
            {
                ndsRunBoundedOpeningPortraitsDraw(tfunc);
            }
            if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
            {
                ndsFinishTaskmanRun();
                return;
            }
        }

        ndsRunBoundedOpeningPortraitsDraw(tfunc);
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindTitle)
    {
        ndsMNTitleRunBoundedUpdates(1u);
        ndsTitleRenderPreview();
        gNdsOpeningMovieTitleResult = NDS_OPENING_MOVIE_TITLE_PASS;
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindVSMode)
    {
        gNdsTaskmanContexts = 2;
        gNdsTaskmanTaskGfxNum = 1;
        gNdsTaskmanGraphicsHeapSize = 0x8000;
        gNdsTaskmanRdpKind = 2;
        gNdsTaskmanRdpBufferSize = 0xC000;
        gNdsTaskmanSceneUpdateSet =
            (sSYTaskmanDefaultFunction.scene_update == gcRunAll) ? 1u : 0u;
        gNdsTaskmanSceneDrawSet =
            (sSYTaskmanDefaultFunction.scene_draw == gcDrawAll) ? 1u : 0u;
        gNdsTaskmanLightsSet =
            (sSYTaskmanDefaultFunction.task_update != NULL) ? 1u : 0u;
        gNdsTaskmanControllerAutoRead = 1;
        gNdsTaskmanDLContextsValid = 2;
        gNdsTaskmanGeneralHeapUsed =
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                  (uintptr_t)gSYTaskmanGeneralHeap.start);
        gNdsTaskmanLoopReached = 1;
#if (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_VS_START_TRANSITION) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_VSBATTLE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_PUPUPU_UPDATE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_MODEL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STRUCT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_INIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_TICK) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_GROUND) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DISPLAY_PROBE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_SCAN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_EXECUTE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_MULTI) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_ALL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_INPUT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_JUMP_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LANDING_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PROCESS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_SCHEDULER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_CONTROLLER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PREVIEW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP)
        ndsMNVSModeRunStartTransitionProbe();

        if ((gNdsVSModeStartTransitionResult ==
                NDS_VS_MODE_START_TRANSITION_PASS) &&
            (gSCManagerSceneData.scene_curr == nSCKindPlayersVS) &&
            (sSYTaskmanStatus == nSYTaskmanStatusLoadScene))
        {
            ndsFinishTaskmanRun();
            return;
        }
#endif
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindPlayersVS)
    {
        gNdsTaskmanContexts = 2;
        gNdsTaskmanTaskGfxNum = 1;
        gNdsTaskmanGraphicsHeapSize = 0x3A98;
        gNdsTaskmanRdpKind = 2;
        gNdsTaskmanRdpBufferSize = 0x8000;
        gNdsTaskmanSceneUpdateSet =
            (sSYTaskmanDefaultFunction.scene_update == gcRunAll) ? 1u : 0u;
        gNdsTaskmanSceneDrawSet =
            (sSYTaskmanDefaultFunction.scene_draw == gcDrawAll) ? 1u : 0u;
        gNdsTaskmanLightsSet =
            (sSYTaskmanDefaultFunction.task_update != NULL) ? 1u : 0u;
        gNdsTaskmanControllerAutoRead = 1;
        gNdsTaskmanDLContextsValid = 2;
        gNdsTaskmanGeneralHeapUsed =
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                  (uintptr_t)gSYTaskmanGeneralHeap.start);
        gNdsTaskmanLoopReached = 1;
#if (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_VSBATTLE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_PUPUPU_UPDATE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_MODEL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STRUCT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_INIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_TICK) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_GROUND) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DISPLAY_PROBE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_SCAN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_EXECUTE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_MULTI) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_ALL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_INPUT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_JUMP_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LANDING_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PROCESS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_SCHEDULER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_CONTROLLER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PREVIEW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP)
        ndsMNPlayersVSRunReadyTransitionProbe();

        if ((gNdsPlayersVSReadyTransitionResult ==
                NDS_PLAYERS_VS_READY_TRANSITION_PASS) &&
            (gSCManagerSceneData.scene_curr == nSCKindMaps) &&
            (sSYTaskmanStatus == nSYTaskmanStatusLoadScene))
        {
            ndsFinishTaskmanRun();
            return;
        }
#endif
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindMaps)
    {
        gNdsTaskmanContexts = 2;
        gNdsTaskmanTaskGfxNum = 1;
        gNdsTaskmanGraphicsHeapSize = 0x8000;
        gNdsTaskmanRdpKind = 2;
        gNdsTaskmanRdpBufferSize = 0x8000;
        gNdsTaskmanSceneUpdateSet =
            (sSYTaskmanDefaultFunction.scene_update == gcRunAll) ? 1u : 0u;
        gNdsTaskmanSceneDrawSet =
            (sSYTaskmanDefaultFunction.scene_draw == gcDrawAll) ? 1u : 0u;
        gNdsTaskmanLightsSet =
            (sSYTaskmanDefaultFunction.task_update != NULL) ? 1u : 0u;
        gNdsTaskmanControllerAutoRead = 1;
        gNdsTaskmanDLContextsValid = 2;
        gNdsTaskmanGeneralHeapUsed =
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                  (uintptr_t)gSYTaskmanGeneralHeap.start);
        gNdsTaskmanLoopReached = 1;
#if (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_VSBATTLE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_PUPUPU_UPDATE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_MODEL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STRUCT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_INIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_TICK) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_GROUND) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DISPLAY_PROBE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_SCAN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_EXECUTE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_MULTI) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_ALL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_INPUT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_JUMP_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LANDING_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PROCESS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_SCHEDULER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_CONTROLLER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PREVIEW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP)
        ndsMNMapsRunSelectVSBattleProbe();

        if ((gNdsMapsSelectTransitionResult ==
                NDS_MAPS_SELECT_TRANSITION_PASS) &&
            (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
            (sSYTaskmanStatus == nSYTaskmanStatusLoadScene))
        {
            ndsFinishTaskmanRun();
            return;
        }
#endif
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;
        gNdsOriginalBootStage |= NDS_BOOT_SCENE_REACHED;
        osStopThread(NULL);
        return;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindVSBattle)
    {
        gNdsTaskmanContexts = 2;
        gNdsTaskmanTaskGfxNum = 1;
        gNdsTaskmanGraphicsHeapSize = 0xD000;
        gNdsTaskmanRdpKind = 2;
        gNdsTaskmanRdpBufferSize = 0xC000;
        gNdsTaskmanSceneUpdateSet =
            (sSYTaskmanDefaultFunction.scene_update != NULL) ? 1u : 0u;
        gNdsTaskmanSceneDrawSet =
            (sSYTaskmanDefaultFunction.scene_draw != NULL) ? 1u : 0u;
        gNdsTaskmanLightsSet =
            (sSYTaskmanDefaultFunction.task_update != NULL) ? 1u : 0u;
        gNdsTaskmanControllerAutoRead = 1;
        gNdsTaskmanDLContextsValid = 2;
        gNdsTaskmanGeneralHeapUsed =
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                  (uintptr_t)gSYTaskmanGeneralHeap.start);
        gNdsTaskmanLoopReached = 1;

#if (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LIVE_PREVIEW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE)
        {
            u32 i;
            u32 update_max;
            u32 is_battle_playable =
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE);
            u32 use_realtime_presentation =
                ((is_battle_playable != 0u) &&
                 (NDS_HARNESS_FAST_LOGIC == 0));

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
            if (is_battle_playable != 0u)
            {
                ndsStageCollisionLoopPrepareRuntime();
#if NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS
                ndsAudioAssetLoadFenced();
#endif
            }
            ndsFighterMarioFoxNaturalMotionPrepare();
#if NDS_IMPORT_BATTLESHIP_IFCOMMON
            if ((is_battle_playable != 0u) &&
                (use_realtime_presentation == 0u))
            {
                gcDrawAll();
                gNdsBattlePlayablePacingDrawCalls++;
            }
#endif
            if (is_battle_playable == 0u)
            {
                update_max = NDS_FIGHTER_NATURAL_MOTION_UPDATE_MAX;
            }
            else if ((use_realtime_presentation == 0u) &&
                     (NDS_DEV_LIVE_INPUT_PREVIEW != 0))
            {
                update_max = NDS_FIGHTER_BATTLE_PLAYABLE_LIVE_UPDATE_MAX;
            }
            else if (use_realtime_presentation == 0u)
            {
                update_max = NDS_FIGHTER_BATTLE_PLAYABLE_UPDATE_MAX;
            }
            else if (NDS_DEV_LIVE_INPUT_PREVIEW != 0)
            {
                update_max = NDS_FIGHTER_BATTLE_PLAYABLE_LIVE_UPDATE_MAX;
            }
            else
            {
                update_max = NDS_FIGHTER_BATTLE_PLAYABLE_REALTIME_SMOKE_UPDATE_MAX;
            }
            if (is_battle_playable != 0u)
            {
                ndsBattlePlayablePacingStart(
                    (NDS_HARNESS_FAST_LOGIC != 0) ? 1u : 0u);
            }
            for (i = 0u; i < update_max;)
            {
                u32 updates_this_iteration = 1u;
                u32 update_in_iteration;
                u32 stop_after_iteration = 0u;
                u32 terminal_update = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                u32 profile_input_ticks = 0u;
                u32 profile_update_ticks = 0u;
                u32 profile_source_update_ticks = 0u;
                u32 profile_audio_update_ticks = 0u;

                sNdsBattlePlayableProfileLoopStartTick = cpuGetTiming();
#endif
                if ((use_realtime_presentation != 0u) &&
                    (gNdsBattlePlayablePacingRestartRequested != 0u))
                {
                    /* The lifecycle verifier requests this only after its
                     * synchronized MATCH_START stop. Reset at an iteration
                     * boundary so debugger time is never reported as a game
                     * slowdown while every natural slip remains visible. */
                    ndsBattlePlayablePacingStart(0u);
                    gNdsBattlePlayablePacingRestartRequested = 0u;
                }
                if (use_realtime_presentation != 0u)
                {
                    /* Smash 64 slows uniformly under load and never repays a
                     * missed retrace with a later logic burst. Run exactly two
                     * unchanged 60 Hz source ticks for each presented frame;
                     * a three-VBlank draw is measured as slowdown, and the
                     * next frame starts cleanly with two more ticks. */
                    updates_this_iteration =
                        NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT;
                }
                for (update_in_iteration = 0u;
                     update_in_iteration < updates_this_iteration;
                     update_in_iteration++)
                {
                    u32 battle_status_before =
                        ((is_battle_playable != 0u) &&
                         (gSCManagerBattleState != NULL)) ?
                            (u32)gSCManagerBattleState->game_status :
                            0xffffffffu;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                    u32 input_start = cpuGetTiming();
#endif
                    NDS_FREEZE_DIAGNOSTICS_MARK(
                        NDS_FREEZE_BREADCRUMB_UPDATE_START);
                    if (use_realtime_presentation != 0u)
                    {
                        (void)ndsPlatformReadInput();
                        if (NDS_DEV_LIVE_INPUT_PREVIEW != 0)
                        {
                            syControllerReadDeviceData();
                            syControllerUpdateGlobalData();
                        }
                        ndsBattlePlayableAdvanceRealtimeLogicClock();
                    }
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                    profile_input_ticks += cpuGetTiming() - input_start;
#endif
                    ndsRunMarioFoxProofUpdate(
                        &gNdsFighterGCRunAllLoopTaskmanUpdateCount);
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                    profile_update_ticks +=
                        gNdsRendererProfileUpdateTicks;
                    profile_source_update_ticks +=
                        gNdsRendererProfileSourceUpdateTicks;
                    profile_audio_update_ticks +=
                        gNdsRendererProfileAudioUpdateTicks;
#endif
                    if ((battle_status_before ==
                         nSCBattleGameStatusWait) &&
                        (gSCManagerBattleState != NULL) &&
                        (gSCManagerBattleState->game_status ==
                         nSCBattleGameStatusGo))
                    {
                        /* The imported countdown GObj performed the source
                         * Wait->Go assignment. Its yellow-only atlas is dead;
                         * release it before arming the post-GO M4 fence. */
                        ndsIFCommonNativeOamReleasePreGoTextures();
                        ndsRendererHardwareArmBattleStaticTextures();
                    }
                    /* BattleShip syTaskmanRunTask checks LoadScene immediately
                     * after task_update and never draws the terminal update. */
                    if ((is_battle_playable != 0u) &&
                        (sSYTaskmanStatus == nSYTaskmanStatusLoadScene))
                    {
                        terminal_update = 1u;
                        break;
                    }
#if NDS_SCENE_MIP_CACHE_LAB
                    if ((is_battle_playable != 0u) &&
                        (use_realtime_presentation != 0u) &&
                        (i == 0u) &&
                        (ndsSceneMipCacheHoldLogic() != FALSE))
                    {
                        u32 seed_guard = 0u;

                        /* Let BattleShip establish the real fighter/camera
                         * state once, then seed without advancing logic or
                         * match time. Seed draws obey the same locked cap. */
                        while ((ndsSceneMipCacheHoldLogic() != FALSE) &&
                               (seed_guard < 4u))
                        {
                            ndsBattlePlayablePresentRealtimeFrame();
                            seed_guard++;
                        }
                        if (ndsSceneMipCacheHoldLogic() != FALSE)
                        {
                            ndsPlatformSceneMipCacheAbort();
                        }
                        ndsBattlePlayablePacingStart(0u);
                    }
#endif
                    if ((is_battle_playable != 0u) &&
                        (use_realtime_presentation == 0u))
                    {
                        ndsBattlePlayableAdvanceFastLogicClock();
                    }
                    if ((is_battle_playable != 0u) &&
                        (use_realtime_presentation == 0u))
                    {
                        gNdsBattlePlayablePacingLogicFrames++;
                    }
                    i++;

                    if (gNdsFighterNaturalMotionResult ==
                        NDS_FIGHTER_NATURAL_MOTION_PASS)
                    {
                        if ((is_battle_playable != 0u) &&
                            (NDS_DEV_LIVE_INPUT_PREVIEW != 0))
                        {
                            continue;
                        }
                        if ((is_battle_playable != 0u) &&
                            (use_realtime_presentation != 0u) &&
                            (i < update_max))
                        {
                            continue;
                        }
#if NDS_IMPORT_BATTLESHIP_AUDIO_BGM
                        if ((is_battle_playable != 0u) &&
                            (use_realtime_presentation == 0u) &&
                            (gNdsAudioBgmElapsedFrames <
                             NDS_AUDIO_BGM_RATE_GUARD_FRAMES))
                        {
                            continue;
                        }
#endif
                        stop_after_iteration = 1u;
                        break;
                    }
                }
                if (terminal_update != 0u)
                {
                    break;
                }
                if (use_realtime_presentation != 0u)
                {
#if NDS_RENDERER_PROFILE_LEVEL >= 1
                    gNdsRendererProfileInputTicks = profile_input_ticks;
                    gNdsRendererProfileUpdateTicks = profile_update_ticks;
                    gNdsRendererProfileSourceUpdateTicks =
                        profile_source_update_ticks;
                    gNdsRendererProfileAudioUpdateTicks =
                        profile_audio_update_ticks;
#endif
                    ndsBattlePlayablePresentRealtimeFrame();
                    /* Count only updates committed to a presented frame. The
                     * source-faithful LoadScene terminal update remains
                     * undrawn and is proven by the lifecycle counters. */
                    gNdsBattlePlayablePacingLogicFrames +=
                        NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT;
                    ndsBattlePlayableFinalizePresentedIteration();
                }
                if (stop_after_iteration != 0u)
                {
                    break;
                }
            }
            if (is_battle_playable != 0u)
            {
                ndsBattlePlayableRecordLifecycleTaskmanExit();
                ndsBattlePlayablePacingFinish();
            }
#if NDS_RENDERER_HW_TRIANGLES && \
    ((NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_COLLISION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
     (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE))
            if ((NDS_DEV_SCENE_HARNESS !=
                    NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE) ||
                ((NDS_HARNESS_FAST_LOGIC != 0) &&
                 (sSYTaskmanStatus != nSYTaskmanStatusLoadScene)))
            {
                ndsFighterMarioFoxStageGCDrawAllLoopSubmitHardwareFrame();
            }
#endif
#else
            u32 live_update_max =
                (NDS_DEV_LIVE_INPUT_PREVIEW != 0) ?
                NDS_FIGHTER_LIVE_PREVIEW_DEV_UPDATE_MAX :
                NDS_FIGHTER_LIVE_PREVIEW_IDLE_UPDATE_MAX;

            ndsFighterMarioFoxSchedulerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterSchedulerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxSchedulerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxControllerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterControllerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxControllerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxPreviewLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterPreviewLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxPreviewLoopResult ==
                    NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxGCRunAllLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_GCRUNALL_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterGCRunAllLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxGCRunAllLoopResult ==
                    NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS)
                {
                    break;
                }
            }

            if ((NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_GCDRAWALL_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_GCDRAWALL_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_COLLISION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_COLLISION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_FLOOR_EDGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPROCESS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPUPDATE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSWEEP_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCROSS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPADJUST_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPEDGE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALL_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPSTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_TURN_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP))
            {
                ndsRunMarioFoxGCRunAllPrerequisiteLoops();
                ndsFighterMarioFoxStageCollisionLoopPrepare();
                ndsFighterMarioFoxStageFloorFollowLoopPrepare();
                ndsFighterMarioFoxStageFloorEdgeLoopPrepare();
                ndsFighterMarioFoxStageMPProcessFloorLoopPrepare();
                ndsFighterMarioFoxStageMPUpdateFloorLoopPrepare();
                ndsFighterMarioFoxStageMPSweepFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCrossFloorLoopPrepare();
                ndsFighterMarioFoxStageMPAdjustFloorLoopPrepare();
                ndsFighterMarioFoxStageMPEdgeFloorLoopPrepare();
                ndsFighterMarioFoxStageMPWallFloorLoopPrepare();
                ndsFighterMarioFoxStageMPStaleFloorLoopPrepare();
                ndsFighterMarioFoxStageMPLiveStaleFloorLoopPrepare();
                ndsFighterMarioFoxStageMPMotionStaleFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffStatusFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffTickFloorLoopPrepare();
                ndsFighterMarioFoxStageMPFallMapFloorLoopPrepare();
                ndsFighterMarioFoxStageMPFallLandFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCeilFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCeilStatusFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffCatchFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffWaitFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffAttackFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffAttackActionLoopPrepare();
                ndsFighterMarioFoxStageMPCliffCommon2LoopPrepare();
                ndsFighterMarioFoxStageMPCliffEscapeActionLoopPrepare();
                ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopPrepare();
                ndsFighterMarioFoxStageMPCliffClimbFloorLoopPrepare();
                ndsFighterMarioFoxStageMPCliffClimbActionLoopPrepare();
                ndsFighterMarioFoxStageMPCliffClimbCommon2LoopPrepare();
                ndsFighterMarioFoxStageMPCliffClimbFinishLoopPrepare();
                ndsFighterMarioFoxStageMPCliffWaitDamageLoopPrepare();
                ndsFighterMarioFoxStageMPPassiveLoopPrepare();
                ndsFighterMarioFoxStageMPDamageRecoverLoopPrepare();
                ndsFighterMarioFoxStageMPLiveHitDamageLoopPrepare();
                ndsFighterMarioFoxStageMPDownWaitLoopPrepare();
                ndsFighterMarioFoxStageTurnLoopPrepare();
                ndsFighterMarioFoxStageMPDownRecoverLoopPrepare();
                ndsFighterMarioFoxStageMPCliffLedgeLoopPrepare();
                ndsFighterMarioFoxStageMPCliffLiveLoopPrepare();
                ndsFighterMarioFoxStageMPWallCopyFloorLoopPrepare();
                ndsFighterMarioFoxStageMPPassFloorLoopPrepare();
                ndsFighterMarioFoxStageMPPlatformFloorLoopPrepare();
                ndsFighterMarioFoxStageMPPlatformTickFloorLoopPrepare();
                ndsFighterMarioFoxStageMPPassInputLoopPrepare();
                ndsFighterMarioFoxStageMPPlatformPosFloorLoopPrepare();
                ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopPrepare();
                ndsFighterMarioFoxStageInishieScaleLoopPrepare();
                ndsFighterMarioFoxStageGCDrawAllLoopPrepare();
                ndsFighterMarioFoxGCDrawAllLoopPrepare();
                for (i = 0u; i < NDS_FIGHTER_GCDRAWALL_LOOP_UPDATE_MAX; i++)
                {
                    scVSBattleFuncUpdate();
                    dSYTaskmanUpdateCount++;
                    gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                    gNdsFighterGCDrawAllLoopTaskmanUpdateCount++;
                    gNdsSCVSBattleOriginalUpdateCount++;
                    gNdsSCVSBattleOriginalUpdateResult =
                        NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                    gNdsSCVSBattleOriginalSetupMask |=
                        NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                    if (gNdsFighterMarioFoxGCDrawAllLoopResult ==
                        NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_PASS)
                    {
#if (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
    (NDS_DEV_SCENE_HARNESS == \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP)
                        ndsFighterMarioFoxStageGCDrawAllLoopFinalize();
                        ndsFighterMarioFoxStageCollisionLoopFinalize();
                        ndsFighterMarioFoxStageFloorFollowLoopFinalize();
                        ndsFighterMarioFoxStageFloorEdgeLoopFinalize();
                        ndsFighterMarioFoxStageMPProcessFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPUpdateFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPSweepFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCrossFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPAdjustFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPEdgeFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPWallFloorLoopFinalize();
#endif
#if (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVESTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPMOTIONSTALE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFTICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLMAP_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPFALLLAND_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEIL_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
    (NDS_DEV_SCENE_HARNESS == \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNWAIT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_TURN_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPWALLCOPY_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_ACTIVE_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_TICK_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPASS_INPUT_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_POS_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPPLATFORM_SPEED_FLOOR_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_INISHIE_SCALE_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_INISHIE_SCALE_LOOP)
                        if (gNdsStageMPLiveStaleFloorLoopSelectedCallbackCount ==
                            0u)
                        {
                            continue;
                        }
#endif
                        ndsFighterMarioFoxStageGCDrawAllLoopFinalize();
                        ndsFighterMarioFoxStageCollisionLoopFinalize();
                        ndsFighterMarioFoxStageFloorFollowLoopFinalize();
                        ndsFighterMarioFoxStageFloorEdgeLoopFinalize();
                        ndsFighterMarioFoxStageMPProcessFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPUpdateFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPSweepFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCrossFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPAdjustFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPEdgeFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPWallFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPStaleFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPLiveStaleFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPMotionStaleFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffStatusFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffTickFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPFallMapFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPFallLandFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCeilFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCeilStatusFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffCatchFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffWaitFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffClimbFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffClimbActionLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffClimbCommon2LoopFinalize();
                        ndsFighterMarioFoxStageMPCliffClimbFinishLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffWaitDamageLoopFinalize();
                        ndsFighterMarioFoxStageMPPassiveLoopFinalize();
                        ndsFighterMarioFoxStageMPDamageRecoverLoopFinalize();
                        ndsFighterMarioFoxStageMPLiveHitDamageLoopFinalize();
                        ndsFighterMarioFoxStageMPDownWaitLoopFinalize();
                        ndsFighterMarioFoxStageTurnLoopFinalize();
                        ndsFighterMarioFoxStageMPDownRecoverLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffLedgeLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffLiveLoopFinalize();
                        ndsFighterMarioFoxStageMPWallCopyFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPPassFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPPlatformFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPPlatformTickFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPPassInputLoopFinalize();
                        ndsFighterMarioFoxStageMPPlatformPosFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopFinalize();
                        ndsFighterMarioFoxStageInishieScaleLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffAttackFloorLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffAttackActionLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffCommon2LoopFinalize();
                        ndsFighterMarioFoxStageMPCliffEscapeActionLoopFinalize();
                        ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopFinalize();
                        break;
                    }
                }
                ndsFighterMarioFoxStageGCDrawAllLoopFinalize();
                ndsFighterMarioFoxStageCollisionLoopFinalize();
                ndsFighterMarioFoxStageFloorFollowLoopFinalize();
                ndsFighterMarioFoxStageFloorEdgeLoopFinalize();
                ndsFighterMarioFoxStageMPProcessFloorLoopFinalize();
                ndsFighterMarioFoxStageMPUpdateFloorLoopFinalize();
                ndsFighterMarioFoxStageMPSweepFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCrossFloorLoopFinalize();
                ndsFighterMarioFoxStageMPAdjustFloorLoopFinalize();
                ndsFighterMarioFoxStageMPEdgeFloorLoopFinalize();
                ndsFighterMarioFoxStageMPWallFloorLoopFinalize();
                ndsFighterMarioFoxStageMPStaleFloorLoopFinalize();
                ndsFighterMarioFoxStageMPLiveStaleFloorLoopFinalize();
                ndsFighterMarioFoxStageMPMotionStaleFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffStatusFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffTickFloorLoopFinalize();
                ndsFighterMarioFoxStageMPFallMapFloorLoopFinalize();
                ndsFighterMarioFoxStageMPFallLandFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCeilFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCeilStatusFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffCatchFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffWaitFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffClimbFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffClimbActionLoopFinalize();
                ndsFighterMarioFoxStageMPCliffClimbCommon2LoopFinalize();
                ndsFighterMarioFoxStageMPCliffClimbFinishLoopFinalize();
                ndsFighterMarioFoxStageMPCliffWaitDamageLoopFinalize();
                ndsFighterMarioFoxStageMPPassiveLoopFinalize();
                ndsFighterMarioFoxStageMPDamageRecoverLoopFinalize();
                ndsFighterMarioFoxStageMPLiveHitDamageLoopFinalize();
                ndsFighterMarioFoxStageMPDownWaitLoopFinalize();
                ndsFighterMarioFoxStageTurnLoopFinalize();
                ndsFighterMarioFoxStageMPDownRecoverLoopFinalize();
                ndsFighterMarioFoxStageMPCliffLedgeLoopFinalize();
                ndsFighterMarioFoxStageMPCliffLiveLoopFinalize();
                ndsFighterMarioFoxStageMPWallCopyFloorLoopFinalize();
                ndsFighterMarioFoxStageMPPassFloorLoopFinalize();
                ndsFighterMarioFoxStageMPPlatformFloorLoopFinalize();
                ndsFighterMarioFoxStageMPPlatformTickFloorLoopFinalize();
                ndsFighterMarioFoxStageMPPassInputLoopFinalize();
                ndsFighterMarioFoxStageMPPlatformPosFloorLoopFinalize();
                ndsFighterMarioFoxStageMPPlatformSpeedFloorLoopFinalize();
                ndsFighterMarioFoxStageInishieScaleLoopFinalize();
                ndsFighterMarioFoxStageMPCliffAttackFloorLoopFinalize();
                ndsFighterMarioFoxStageMPCliffAttackActionLoopFinalize();
                ndsFighterMarioFoxStageMPCliffCommon2LoopFinalize();
                ndsFighterMarioFoxStageMPCliffEscapeActionLoopFinalize();
                ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopFinalize();
            }

            if ((NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LIVE_PREVIEW) ||
                (NDS_DEV_SCENE_HARNESS ==
                    NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LIVE_PREVIEW))
            {
                ndsFighterMarioFoxLivePreviewPrepare();
                for (i = 0u; i < live_update_max; i++)
                {
                    scVSBattleFuncUpdate();
                    dSYTaskmanUpdateCount++;
                    gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                    gNdsFighterLivePreviewTaskmanUpdateCount++;
                    gNdsSCVSBattleOriginalUpdateCount++;
                    gNdsSCVSBattleOriginalUpdateResult =
                        NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                    gNdsSCVSBattleOriginalSetupMask |=
                        NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                    if ((NDS_DEV_LIVE_INPUT_PREVIEW == 0) &&
                        (gNdsFighterMarioFoxLivePreviewResult ==
                         NDS_FIGHTER_MARIOFOX_LIVE_PREVIEW_PASS))
                    {
                        break;
                    }
                }
            }
#endif
        }
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_MODEL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_MODEL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STRUCT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STRUCT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_INIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_INIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WAIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WAIT_TICK) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_TICK) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WAIT_GROUND) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WAIT_GROUND) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DISPLAY_PROBE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DISPLAY_PROBE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_SCAN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_SCAN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_EXECUTE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_EXECUTE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_DRAW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_DRAW_MULTI) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_MULTI) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DL_DRAW_ALL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DL_DRAW_ALL) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WALK_INPUT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_INPUT) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_WALK_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_WALK_LOOP)
        ndsRunMarioFoxProofUpdate(NULL);
        ndsFighterMarioFoxRunImmediateProofChain();
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP)
        {
            u32 i;

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
            ndsFighterMarioFoxNaturalMotionPrepare();
            for (i = 0u; i < NDS_FIGHTER_NATURAL_MOTION_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterGCRunAllLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterNaturalMotionResult ==
                    NDS_FIGHTER_NATURAL_MOTION_PASS)
                {
                    break;
                }
            }
#else
            ndsRunMarioFoxProcessPrerequisiteLoop();
            ndsFighterMarioFoxSchedulerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterSchedulerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxSchedulerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxControllerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterControllerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxControllerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxPreviewLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterPreviewLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxPreviewLoopResult ==
                    NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxGCRunAllLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_GCRUNALL_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterGCRunAllLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxGCRunAllLoopResult ==
                    NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS)
                {
                    break;
                }
            }
#endif
        }
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_PREVIEW_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PREVIEW_LOOP)
        {
            u32 i;

            ndsRunMarioFoxProcessPrerequisiteLoop();
            ndsFighterMarioFoxSchedulerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterSchedulerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxSchedulerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxControllerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterControllerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxControllerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxPreviewLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterPreviewLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxPreviewLoopResult ==
                    NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS)
                {
                    break;
                }
            }
        }
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_CONTROLLER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_CONTROLLER_LOOP)
        {
            u32 i;

            ndsRunMarioFoxProcessPrerequisiteLoop();
            ndsFighterMarioFoxSchedulerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterSchedulerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxSchedulerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
                {
                    break;
                }
            }

            ndsFighterMarioFoxControllerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterControllerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxControllerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS)
                {
                    break;
                }
            }
        }
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_SCHEDULER_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_SCHEDULER_LOOP)
        {
            u32 i;

            ndsRunMarioFoxProcessPrerequisiteLoop();
            ndsFighterMarioFoxSchedulerLoopPrepare();
            for (i = 0u; i < NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterSchedulerLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterMarioFoxSchedulerLoopResult ==
                    NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS)
                {
                    break;
                }
            }
        }
#elif (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_JUMP_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_JUMP_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_LANDING_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_LANDING_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_PROCESS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_PROCESS_LOOP)
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && \
    ((NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DASH_RUN) || \
     (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN))
        {
            u32 i;

            ndsFighterMarioFoxNaturalMotionPrepare();
            for (i = 0u; i < NDS_FIGHTER_NATURAL_MOTION_UPDATE_MAX; i++)
            {
                scVSBattleFuncUpdate();
                dSYTaskmanUpdateCount++;
                gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
                gNdsFighterGCRunAllLoopTaskmanUpdateCount++;
                gNdsSCVSBattleOriginalUpdateCount++;
                gNdsSCVSBattleOriginalUpdateResult =
                    NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
                gNdsSCVSBattleOriginalSetupMask |=
                    NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;

                if (gNdsFighterNaturalMotionResult ==
                    NDS_FIGHTER_NATURAL_MOTION_PASS)
                {
                    break;
                }
            }
        }
#else
        ndsRunMarioFoxProcessPrerequisiteLoop();
#endif
#else
        scVSBattleFuncUpdate();
        dSYTaskmanUpdateCount++;
        gNdsTaskmanBoundedUpdateCount = dSYTaskmanUpdateCount;
        gNdsSCVSBattleOriginalUpdateCount++;
        gNdsSCVSBattleOriginalUpdateResult =
            NDS_SCVSBATTLE_ORIGINAL_UPDATE_PASS;
        gNdsSCVSBattleOriginalSetupMask |=
            NDS_SCVSBATTLE_SETUP_TASKMAN_UPDATE_READY;
#endif

#if (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PUPUPU_UPDATE) || \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_MENU_CHAIN_PUPUPU_UPDATE)
        ndsGRPupupuRunSafeUpdateProbe();
        if (gNdsPupupuUpdateResult == NDS_PUPUPU_UPDATE_PASS)
        {
            gNdsPupupuGroundDeferredMask |= 1u << 3;
            gNdsPupupuGroundDeferredMask |= 1u << 4;
        }
#endif

        ndsFinishTaskmanRun();
        gNdsSceneBoundaryKind = gSCManagerSceneData.scene_curr;
        gNdsSceneBoundaryResult = NDS_SCENE_BOUNDARY_PASS;

#if NDS_DEV_LIVE_INPUT_PREVIEW
        if (sSYTaskmanStatus == nSYTaskmanStatusLoadScene)
        {
            /* The original task loop returns after its cleanup tail. Resume
             * scVSBattleStartScene so it can score and select VS Results. */
            return;
        }
#endif
        osStopThread(NULL);
        return;
    }

    /* The real taskman scene setup populated sSYTaskmanDefaultFunction and the
     * DL/heap/RDP state during syTaskmanLoadScene; reflect it here as proof. */
    gNdsTaskmanContexts = 2;
    gNdsTaskmanTaskGfxNum = 1;
    gNdsTaskmanGraphicsHeapSize = 0x2800;
    gNdsTaskmanRdpKind = 2;
    gNdsTaskmanRdpBufferSize = 0xC000;
    gNdsTaskmanSceneUpdateSet = (sSYTaskmanDefaultFunction.scene_update == gcRunAll) ? 1u : 0u;
    gNdsTaskmanSceneDrawSet = (sSYTaskmanDefaultFunction.scene_draw == gcDrawAll) ? 1u : 0u;
    gNdsTaskmanLightsSet = (sSYTaskmanDefaultFunction.task_update != NULL) ? 1u : 0u;
    gNdsTaskmanControllerAutoRead = 1;
    gNdsTaskmanDLContextsValid = 2;
    gNdsTaskmanGeneralHeapUsed =
        (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
              (uintptr_t)gSYTaskmanGeneralHeap.start);
    /* Snapshot the real startup object state now that func_start has run. */
    ndsCaptureRealStartupState();

    /* Execute bounded real task updates (controller read + gcRunAll) and one
     * bounded startup draw. This advances the original object/update coroutine
     * and scene-transition paths without entering an unbounded renderer loop. */
    ndsRunBoundedTaskmanUpdates(tfunc);

    gNdsTaskmanBridgeResult = NDS_TASKMAN_BRIDGE_PASS;
    gNdsTaskmanLoopReached = 1;
    gNdsStartupTaskmanMallocCount = gNdsTaskmanMallocCount;
    ndsFinishTaskmanRun();
}
