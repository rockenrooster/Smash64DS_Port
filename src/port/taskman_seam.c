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

