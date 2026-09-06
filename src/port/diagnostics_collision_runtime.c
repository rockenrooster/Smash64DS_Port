void ndsCollisionRuntimeDiagnosticsReset(void)
{
    NDSCollisionRuntimeDiagnostics reset = { 0 };

    reset.topology_last_line = -1;
    reset.topology_last_prev = -1;
    reset.topology_last_next = -1;
    reset.damage_last_line = -1;
    reset.damage_last_status = -1;
    reset.fireball_last_line = -1;
    gNdsCollisionRuntimeDiagnostics = reset;
}

volatile u32 gNdsFighterProjectileProofResult;
volatile u32 gNdsFighterProjectileProofMask;
volatile u32 gNdsFighterProjectileProofActorSlot;
volatile u32 gNdsFighterProjectileProofActorKind;
volatile u32 gNdsFighterProjectileProofBPressFrames;
volatile u32 gNdsFighterProjectileProofSpecialStatusFrames;
volatile u32 gNdsFighterProjectileProofSpecialMotion;
volatile u32 gNdsFighterProjectileProofAccessoryFrames;
volatile u32 gNdsFighterProjectileProofFlag0Frames;
/* BUGS.md "Fox's pistol model is missing": engagement proof for the model-part
 * state half. ftmain.c:575 dispatches nFTMotionEventSetModelPartID into
 * ftParamSetModelPartID, which used to discard it. OnCount rising on the gate
 * arm is what proves the gun is being REQUESTED; it is the precondition for
 * the texture ever being asked for, and therefore for capturing its key. */
/* Fire hits that reached the element dispatch at ftmain.c:2713/2771/2808.
 * The nEFKind path is dead in P1, so this is the only way fire is requested. */
volatile u32 gNdsFighterDamageFireCallCount;
volatile u32 gNdsFighterEffectKindMask0;
volatile u32 gNdsFighterEffectKindMask1;
volatile u32 gNdsFighterEffectKindMask2;
volatile u32 gNdsFighterEffectKindMask3;
volatile u32 gNdsFighterModelPartSetCount;
volatile u32 gNdsFighterModelPartOnCount;
volatile u32 gNdsFighterModelPartResetCount;
volatile u32 gNdsFighterProjectileProofSpawnCallCount;
volatile u32 gNdsFighterProjectileProofSpawnSuccessCount;
volatile u32 gNdsFighterProjectileProofUpdateDestroyCount;
volatile u32 gNdsFighterProjectileProofMapDestroyCount;
volatile u32 gNdsFighterProjectileProofHitDestroyCount;
volatile u32 gNdsFighterProjectileProofWeaponFrames;
volatile u32 gNdsFighterProjectileProofWeaponCountMax;
volatile u32 gNdsFighterProjectileProofSpawnFailGObjCount;
volatile u32 gNdsFighterProjectileProofSpawnFailPoolCount;
volatile u32 gNdsFighterProjectileProofSpawnFailGObjMax;
volatile u32 gNdsFighterProjectileProofSpawnFailGObjActive;
volatile u32 gNdsFighterProjectileProofSpawnFailHeapFree;
volatile u32 gNdsWeaponStructBytes;
volatile u32 gNdsWeaponPoolEntries;
/* Starts at all-ones so the first presented frame sets a real low-water. */
volatile u32 gNdsTaskmanGeneralHeapFreeMin = 0xffffffffu;
volatile u32 gNdsGCDrawsActiveMax;
volatile u32 gNdsEffectPoolDepth;
/* Same convention as the heap low-water above. */
volatile u32 gNdsEffectPoolFreeMin = 0xffffffffu;
volatile u32 gNdsEFDescResolveCount;
volatile u32 gNdsEFDescDisabledCount;
volatile u32 gNdsEFDescUnknownFileCount;
volatile u32 gNdsEFDescDisabledLast;
volatile u32 gNdsEFDescUnknownFileLast;
volatile u32 gNdsEFDescDeferRecoverCount;
volatile u32 gNdsEFDescDeferOverflowCount;
volatile u32 gNdsEFDescEffectsSpan[3];
volatile u32 gNdsKOBurstStage;
volatile u32 gNdsKOBurstAttemptCount;
volatile u32 gNdsKOBurstCompleteCount;
volatile u32 gNdsKOBurstDropMask;
volatile u32 gNdsFighterProjectileProofKindMask;
volatile u32 gNdsFighterProjectileProofAttackStateMask;
volatile u32 gNdsFighterProjectileProofDamageMax;
volatile u32 gNdsFighterProjectileProofLifetimeMax;
volatile u32 gNdsFighterProjectileProofMapMask;
volatile s32 gNdsFighterProjectileProofFireballIndex;
volatile s32 gNdsFighterProjectileProofFireballInitialVelXMilli;
volatile s32 gNdsFighterProjectileProofFireballInitialVelYMilli;
volatile u32 gNdsFighterReflectorProofResult;
volatile u32 gNdsFighterReflectorProofMask;
volatile u32 gNdsFighterReflectorProofFoxSlot;
volatile u32 gNdsFighterReflectorProofProjectileSlot;
volatile u32 gNdsFighterReflectorProofDownBPressFrames;
volatile u32 gNdsFighterReflectorProofStartFrames;
volatile u32 gNdsFighterReflectorProofLoopFrames;
volatile u32 gNdsFighterReflectorProofHitFrames;
volatile u32 gNdsFighterReflectorProofIsReflectFrames;
volatile s32 gNdsFighterReflectorProofReflectLRBeforeHit;
volatile u32 gNdsFighterReflectorProofReflectLRClearFrames;
volatile u32 gNdsFighterReflectorProofHitSetCallCount;
volatile u32 gNdsFighterReflectorProofFireballProcCount;
volatile s32 gNdsFighterReflectorProofFireballVelXBefore;
volatile s32 gNdsFighterReflectorProofFireballVelXAfter;
volatile u32 gNdsFighterReflectorProofFireballOwnerKind;
volatile u32 gNdsFighterReflectorProofFireballCanReflect;
volatile u32 gNdsFighterReflectorProofFireballCanAbsorb;
volatile u32 gNdsFighterReflectorProofFireballCanShield;
volatile u32 gNdsFighterReflectorProofFireballAttackCount;
volatile u32 gNdsFighterReflectorProofFireballDamage;
volatile u32 gNdsFighterReflectorProofFireballSizeMilli;
volatile s32 gNdsFighterReflectorProofFireballDXMilli;
volatile s32 gNdsFighterReflectorProofFireballDYMilli;
volatile u32 gNdsFighterReflectorProofSpecialSizeMilli;
volatile u32 gNdsFighterReflectorProofSpecialResist;
volatile u32 gNdsFighterSpecialsProofMask;
volatile u32 gNdsFighterSpecialsProofPhase;
volatile u32 gNdsFighterSpecialsProofPhaseFrames;
volatile u32 gNdsFighterSpecialsMarioSlot;
volatile u32 gNdsFighterSpecialsFoxSlot;
volatile u32 gNdsFighterSpecialsMarioHiPressFrames;
volatile u32 gNdsFighterSpecialsMarioHiFrames;
volatile u32 gNdsFighterSpecialsMarioAirHiFrames;
volatile u32 gNdsFighterSpecialsMarioFallSpecialFrames;
volatile u32 gNdsFighterSpecialsMarioLandingFallSpecialFrames;
volatile u32 gNdsFighterSpecialsMarioHiWaitFrames;
volatile s32 gNdsFighterSpecialsMarioHiRootYMilli;
volatile u32 gNdsFighterSpecialsMarioHiDamageMax;
volatile u32 gNdsFighterSpecialsMarioLwPressFrames;
volatile u32 gNdsFighterSpecialsMarioLwFrames;
volatile u32 gNdsFighterSpecialsMarioAirLwFrames;
volatile u32 gNdsFighterSpecialsMarioLwDustEffectCount;
volatile u32 gNdsFighterSpecialsMarioLwWaitFrames;
volatile u32 gNdsFighterSpecialsMarioLwDamageMax;
volatile u32 gNdsFighterSpecialsFoxHiPressFrames;
volatile u32 gNdsFighterSpecialsFoxHiStartFrames;
volatile u32 gNdsFighterSpecialsFoxHiHoldFrames;
volatile u32 gNdsFighterSpecialsFoxHiTravelFrames;
volatile u32 gNdsFighterSpecialsFoxHiEndFrames;
volatile u32 gNdsFighterSpecialsFoxHiBoundFrames;
volatile u32 gNdsFighterSpecialsFoxHiWaitFrames;
volatile s32 gNdsFighterSpecialsFoxHiRootYMilli;
#if NDS_P2_DONKEY
volatile u32 gNdsFighterDonkeySpecialsSlot;
volatile u32 gNdsFighterDonkeySpecialsNChargePressFrames;
volatile u32 gNdsFighterDonkeySpecialsNStartFrames;
volatile u32 gNdsFighterDonkeySpecialsNLoopFrames;
volatile u32 gNdsFighterDonkeySpecialsNStorePressFrames;
volatile u32 gNdsFighterDonkeySpecialsNStoredChargeMax;
volatile u32 gNdsFighterDonkeySpecialsNStoredWaitFrames;
volatile u32 gNdsFighterDonkeySpecialsNResumePressFrames;
volatile u32 gNdsFighterDonkeySpecialsNReleaseTapFrames;
volatile u32 gNdsFighterDonkeySpecialsNEndFrames;
volatile u32 gNdsFighterDonkeySpecialsNReleaseChargeMax;
volatile u32 gNdsFighterDonkeySpecialsNPassiveResetFrames;
volatile u32 gNdsFighterDonkeySpecialsNReleaseWaitFrames;
volatile u32 gNdsFighterDonkeySpecialsHiPressFrames;
volatile u32 gNdsFighterDonkeySpecialsHiFrames;
volatile u32 gNdsFighterDonkeySpecialsHiGroundGAFrames;
volatile u32 gNdsFighterDonkeySpecialsHiWaitFrames;
volatile u32 gNdsFighterDonkeySpecialsLwPressFrames;
volatile u32 gNdsFighterDonkeySpecialsLwStartFrames;
volatile u32 gNdsFighterDonkeySpecialsLwLoopFrames;
volatile u32 gNdsFighterDonkeySpecialsLwRepeatPressFrames;
volatile u32 gNdsFighterDonkeySpecialsLwLoopFlagFrames;
volatile u32 gNdsFighterDonkeySpecialsLwEndFrames;
volatile u32 gNdsFighterDonkeySpecialsLwWaitFrames;
#endif
#if NDS_P2_SAMUS
volatile u32 gNdsFighterSamusSpecialsSlot;
volatile u32 gNdsFighterSamusSpecialsNPressFrames;
volatile u32 gNdsFighterSamusSpecialsNStartFrames;
volatile u32 gNdsFighterSamusSpecialsNLoopFrames;
volatile u32 gNdsFighterSamusSpecialsNChargeMax;
volatile u32 gNdsFighterSamusSpecialsNFullWaitFrames;
volatile u32 gNdsFighterSamusSpecialsNReleasePressFrames;
volatile u32 gNdsFighterSamusSpecialsNEndFrames;
volatile u32 gNdsFighterSamusSpecialsNReleaseWaitFrames;
volatile u32 gNdsFighterSamusSpecialsLwPressFrames;
volatile u32 gNdsFighterSamusSpecialsLwFrames;
volatile u32 gNdsFighterSamusSpecialsLwWaitFrames;
#endif
volatile u32 gNdsFighterNaturalMovesetMask;
#if NDS_P2_SAMUS_STATE_TOUR
volatile u32 gNdsSamusStateTourMask;
volatile u32 gNdsSamusStateTourPhase;
volatile u32 gNdsSamusStateTourPhaseFrames;
volatile u32 gNdsSamusStateTourStatus;
volatile u32 gNdsSamusStateTourMotion;
volatile u32 gNdsSamusStateTourCliffID;
volatile u32 gNdsSamusStateTourStageCount;
#endif
#if NDS_P2_SAMUS_TUMBLE_TOUR
volatile u32 gNdsSamusTumbleTourMask;
volatile u32 gNdsSamusTumbleTourDamageFlyMask;
volatile u32 gNdsSamusTumbleTourScenario;
volatile u32 gNdsSamusTumbleTourStep;
volatile u32 gNdsSamusTumbleTourFrames;
volatile u32 gNdsSamusTumbleTourStatus;
volatile u32 gNdsSamusTumbleTourMotion;
volatile u32 gNdsSamusTumbleTourHitCount;
volatile u32 gNdsSamusTumbleTourStageCount;
volatile u32 gNdsSamusTumbleTourDone;
#endif
#if NDS_P2_SAMUS_DAMAGEFLY_TOUR
volatile u32 gNdsSamusDamageFlyTourMask;
volatile u32 gNdsSamusDamageFlyTourScenario;
volatile u32 gNdsSamusDamageFlyTourStep;
volatile u32 gNdsSamusDamageFlyTourFrames;
volatile u32 gNdsSamusDamageFlyTourStatus;
volatile u32 gNdsSamusDamageFlyTourMotion;
volatile u32 gNdsSamusDamageFlyTourAttackerMask;
volatile u32 gNdsSamusDamageFlyTourPlacementPacked;
volatile u32 gNdsSamusDamageFlyTourHitCount;
volatile u32 gNdsSamusDamageFlyTourRollAttempts;
volatile u32 gNdsSamusDamageFlyTourSakuraiHitCount;
volatile u32 gNdsSamusDamageFlyTourTopAngle80Count;
volatile u32 gNdsSamusDamageFlyTourRollPercent;
volatile u32 gNdsSamusDamageFlyTourMismatchCount;
volatile u32 gNdsSamusDamageFlyTourStageCount;
volatile u32 gNdsSamusDamageFlyTourTerminalCount;
volatile u32 gNdsSamusDamageFlyTourDone;
#endif
#if NDS_P2_SAMUS_ATTACK_TOUR
volatile u32 gNdsSamusAttackTourMask;
volatile u32 gNdsSamusAttackTourScenario;
volatile u32 gNdsSamusAttackTourStep;
volatile u32 gNdsSamusAttackTourFrames;
volatile u32 gNdsSamusAttackTourStatus;
volatile u32 gNdsSamusAttackTourMotion;
volatile u32 gNdsSamusAttackTourStageCount;
volatile u32 gNdsSamusAttackTourTerminalCount;
volatile u32 gNdsSamusAttackTourCatchAttr;
volatile u32 gNdsSamusAttackTourGrabInputCount;
volatile u32 gNdsSamusAttackTourCatchStatusMask;
volatile u32 gNdsSamusAttackTourCatchFrames;
volatile u32 gNdsSamusAttackTourCatchActiveFrames;
volatile u32 gNdsSamusAttackTourCatchSearchFrames;
volatile u32 gNdsSamusAttackTourCatchAttackMask;
volatile u32 gNdsSamusAttackTourCatchAnimFrameMaxMilli;
volatile u32 gNdsSamusAttackTourVictimGrabbableMask;
volatile u32 gNdsSamusAttackTourVictimNormalMask;
volatile u32 gNdsSamusAttackTourJoint36SeenCount;
volatile u32 gNdsSamusAttackTourJoint36AttackMask;
volatile s32 gNdsSamusAttackTourMinGrabDXMilli;
volatile s32 gNdsSamusAttackTourGrab0XMilli;
volatile s32 gNdsSamusAttackTourGrab1XMilli;
volatile s32 gNdsSamusAttackTourFoxXMilli;
volatile u32 gNdsSamusAttackTourDone;
#endif
volatile u32 gNdsFighterNaturalMovesetPhase;
volatile u32 gNdsFighterNaturalMovesetPhaseFrames;
volatile u32 gNdsFighterNaturalMovesetTiltS3Frames;
volatile u32 gNdsFighterNaturalMovesetTiltHi3Frames;
volatile u32 gNdsFighterNaturalMovesetTiltLw3Frames;
volatile u32 gNdsFighterNaturalMovesetTiltHitboxFrames;
volatile u32 gNdsFighterNaturalMovesetSmashFrames;
volatile u32 gNdsFighterNaturalMovesetSmashHitboxFrames;
volatile u32 gNdsFighterNaturalMovesetAerialFrames;
volatile u32 gNdsFighterNaturalMovesetAerialHitboxFrames;
volatile u32 gNdsFighterNaturalMovesetLandingFrames;
volatile u32 gNdsFighterNaturalMovesetCatchFrames;
volatile u32 gNdsFighterNaturalMovesetCatchWaitFrames;
volatile u32 gNdsFighterNaturalMovesetThrowFrames;
volatile u32 gNdsFighterNaturalMovesetThrownFrames;
volatile u32 gNdsFighterNaturalMovesetThrowRecoverFrames;
volatile u32 gNdsFighterNaturalMovesetThrowDamageBefore;
volatile u32 gNdsFighterNaturalMovesetThrowDamageAfter;
volatile u32 gNdsFighterNaturalMovesetAttackerStatus;
volatile u32 gNdsFighterNaturalMovesetAttackerMotion;
volatile u32 gNdsFighterNaturalMovesetAttackerGA;
volatile s32 gNdsFighterNaturalMovesetAttackerRootYMilli;
volatile u32 gNdsFighterNaturalMovesetVictimStatus;
volatile u32 gNdsFighterNaturalMovesetVictimMotion;
volatile u32 gNdsFighterNaturalMovesetVictimGA;
volatile s32 gNdsFighterNaturalMovesetVictimRootYMilli;
volatile u32 gNdsFighterBattlePlayableResult;
volatile u32 gNdsFighterBattlePlayableMask;
volatile u32 gNdsFighterBattlePlayableVictimSlot;
volatile u32 gNdsFighterBattlePlayableVictimStockStart;
volatile u32 gNdsFighterBattlePlayableVictimStockFinal;
volatile u32 gNdsFighterBattlePlayableBattleStockStart;
volatile u32 gNdsFighterBattlePlayableBattleStockFinal;
volatile u32 gNdsFighterBattlePlayableFallsStart;
volatile u32 gNdsFighterBattlePlayableFallsFinal;
volatile u32 gNdsFighterBattlePlayableDeadFrames;
volatile u32 gNdsFighterBattlePlayableRebirthDownFrames;
volatile u32 gNdsFighterBattlePlayableRebirthStandFrames;
volatile u32 gNdsFighterBattlePlayableRebirthWaitFrames;
volatile u32 gNdsFighterBattlePlayableFallAfterRebirthFrames;
volatile u32 gNdsFighterBattlePlayableWaitAfterRebirthFrames;
volatile u32 gNdsFighterBattlePlayableFinalStatus;
volatile u32 gNdsFighterBattlePlayableFinalGA;
volatile u32 gNdsFighterBattlePlayableFinalFloor;
volatile u32 gNdsFighterBattlePlayableFinalIsRebirth;
volatile u32 gNdsFighterBattlePlayableFinalIsGhost;
volatile u32 gNdsFighterBattlePlayableFinalCameraMode;
volatile u32 gNdsFighterBattlePlayableKOStickFrames;
volatile u32 gNdsFighterBattlePlayableMapCallCount;
volatile u32 gNdsFighterBattlePlayableMapHitCount;
volatile u32 gNdsFighterBattlePlayableMapFloorHitCount;
volatile u32 gNdsFighterBattlePlayableMapCliffHitCount;
volatile u32 gNdsFighterBattlePlayableMapCeilHitCount;
volatile u32 gNdsFighterBattlePlayableMapLastMaskStat;
volatile u32 gNdsFighterBattlePlayableMapLastMaskCurr;
volatile s32 gNdsFighterBattlePlayableFinalXMilli;
volatile s32 gNdsFighterBattlePlayableFinalYMilli;
volatile s32 gNdsFighterBattlePlayableFinalVelXMilli;
volatile s32 gNdsFighterBattlePlayableFinalVelYMilli;
volatile s32 gNdsFighterBattlePlayableFinalFloorDistMilli;
volatile u32 gNdsFTComputerSetupCount;
volatile u32 gNdsFTComputerDamageDetectCount;
volatile u32 gNdsFTComputerProcessCount;
volatile u32 gNdsFTComputerTargetFrames;
volatile u32 gNdsFTComputerObjectiveMask;
volatile u32 gNdsFTComputerBehaviorMask;
volatile u32 gNdsFTComputerInputChangeCount;
volatile u32 gNdsFTComputerStickFrames;
volatile u32 gNdsFTComputerButtonAFrames;
volatile u32 gNdsFTComputerButtonBFrames;
volatile u32 gNdsFTComputerButtonZFrames;
volatile u32 gNdsFTComputerAttackFrames;
volatile u32 gNdsFTComputerHitboxFrames;
volatile u32 gNdsFTComputerGuardFrames;
volatile u32 gNdsFTComputerRecoveryFrames;
volatile u32 gNdsFTComputerStatusChangeCount;
volatile u32 gNdsFTComputerFinalStatus;
volatile u32 gNdsFTComputerFinalGA;
volatile u32 gNdsFTComputerFinalInputKind;
volatile u32 gNdsFTComputerMarioDamageMax;
volatile u32 gNdsFTComputerFloorLineCount;
volatile s32 gNdsFTComputerStartXMilli;
volatile s32 gNdsFTComputerMinXMilli;
volatile s32 gNdsFTComputerMaxXMilli;
volatile s32 gNdsFTComputerFinalXMilli;
volatile u32 gNdsBattlePlayablePacingResult;
volatile u32 gNdsBattlePlayablePacingMode;
volatile u32 gNdsBattlePlayablePacingLogicFrames;
volatile u32 gNdsBattlePlayablePacingUpdateBase;
volatile u32 gNdsBattlePlayablePacingPresentedFrames;
volatile u32 gNdsBattlePlayablePacingDrawCalls;
volatile u32 gNdsBattlePlayablePacingTimerTicks;
volatile u32 gNdsBattlePlayablePacingPresentFpsX10;
volatile u32 gNdsBattlePlayablePacingLogicFpsX10;
volatile u32 gNdsBattlePlayablePacingVBlankStart;
volatile u32 gNdsBattlePlayablePacingVBlanks;
volatile u32 gNdsBattlePlayablePacingRestartRequested;
volatile u32 gNdsBattlePlayablePacingPresentIntervalMin;
volatile u32 gNdsBattlePlayablePacingPresentIntervalMax;
/* Per-VBlank-interval presentation counts. Indices 0 and 1 are unused
 * (impossible under the locked-2-present scheduler); 2/3/4 are populated
 * directly and index 5 is the "5 or more VBlanks" bucket. Device A/B reports
 * read this histogram, never min FPS, because min FPS is discontinuous across
 * the 4->5 VBlank boundary. See AGENTS.md. */
volatile u32 gNdsBattlePlayablePacingPresentIntervalBucket[
    NDS_BATTLE_PLAYABLE_PACING_INTERVAL_BUCKET_COUNT];
volatile u32 gNdsBattlePlayablePacingCadenceViolationCount;
volatile u32 gNdsBattlePlayablePacingPhasePresentCount[
    NDS_BATTLE_PLAYABLE_PACING_PHASE_COUNT];
volatile u32 gNdsBattlePlayablePacingPhaseSlipCount[
    NDS_BATTLE_PLAYABLE_PACING_PHASE_COUNT];
#if (NDS_HARNESS_FAST_LOGIC == 0) && \
    (NDS_RENDERER_HW_TRIANGLES != 0) && \
    (NDS_DEV_LIVE_INPUT_PREVIEW != 0)
volatile u32 gNdsBuildModeCanonicalWord = NDS_BUILD_MODE_CANO_WORD;
volatile u32 gNdsBuildModeShippedWord = NDS_BUILD_MODE_SHIP_WORD;
#else
volatile u32 gNdsBuildModeCanonicalWord;
volatile u32 gNdsBuildModeShippedWord;
#endif
#if NDS_HARNESS_FAST_LOGIC != 0
volatile u32 gNdsBuildModeFastWord = NDS_BUILD_MODE_FAST_WORD;
#else
volatile u32 gNdsBuildModeFastWord;
#endif
volatile u32 gNdsRendererProfileFrameCount;
/* Keep the immutable benchmark-build identity beside the already-live profile
 * word so --gc-sections retains both without any per-frame touch. */
volatile u32 gNdsRendererProfileLevel
    __attribute__((section(".data.nds_renderer_runtime_identity"))) =
        NDS_RENDERER_PROFILE_LEVEL;
volatile u32 gNdsRendererM2DetailedLedger
    __attribute__((section(".data.nds_renderer_runtime_identity"))) =
        NDS_RENDERER_M2_DETAILED_LEDGER;
volatile u32 gNdsRendererDepthStageSamples;
volatile s32 gNdsRendererDepthStageMin;
volatile s32 gNdsRendererDepthStageMax;
volatile s32 gNdsRendererDepthStageWMin;
volatile s32 gNdsRendererDepthStageWMax;
volatile u32 gNdsRendererDepthFighterP0Samples;
volatile s32 gNdsRendererDepthFighterP0Min;
volatile s32 gNdsRendererDepthFighterP0Max;
volatile s32 gNdsRendererDepthFighterP0WMin;
volatile s32 gNdsRendererDepthFighterP0WMax;
volatile u32 gNdsRendererDepthFighterP1Samples;
volatile s32 gNdsRendererDepthFighterP1Min;
volatile s32 gNdsRendererDepthFighterP1Max;
volatile s32 gNdsRendererDepthFighterP1WMin;
volatile s32 gNdsRendererDepthFighterP1WMax;
volatile u32 gNdsRendererProfileUpdateTicks;
volatile u32 gNdsRendererProfilePresentTicks;
volatile u32 gNdsRendererProfileDrawTicks;
volatile u32 gNdsRendererProfileHudTicks;
#if NDS_TICK_HUD
volatile u32 gNdsTickHudBuckets[nNDSTickHudBucketCount];
volatile u32 gNdsTickHudFighterTicks;
volatile u32 gNdsTickHudStageTicks;
volatile u32 gNdsTickHudBackgroundTicks;
volatile u32 gNdsTickHudForegroundTicks;
volatile u32 gNdsTickHudAudioTicks;
volatile u32 gNdsTickHudSourceTicks;
/* Cycle 85 SRC split. SRC owns 68.9% of the both-CPU gate arm's WORK-H
 * excursion (board, cycle 80) and a residual cannot be optimised, so these name
 * what is inside it: SHDT is live-hitbox hit detection
 * (ftMainProcSearchHitAll), SWRM is the anim-cache warm step
 * (ndsR2AnimCachePreloadStep, the one asset load inside SRC). Both spans are
 * nested inside the SRC bracket in ndsRunMarioFoxProofUpdate, so they are
 * published as their own ring buckets but NOT added to `named`. */
volatile u32 gNdsTickHudSrcHitDetectTicks;
volatile u32 gNdsTickHudSrcAnimWarmTicks;
/* Cycle 86 SBAS split. GCRA is gcRunAll -- the sole gateway to the whole
 * simulation inside SRC (decomp ifcommon.c:2970) -- and SCPU/SCAT/SPRM are three
 * of the six per-fighter procs, each bracketed on an existing port wrapper so no
 * decomp/ edit was needed. Nested inside SRC exactly like the cycle-85 pair, so
 * they are published after `named` is summed and never added to it. */
volatile u32 gNdsTickHudSrcRunAllTicks;
volatile u32 gNdsTickHudSrcComputerTicks;
volatile u32 gNdsTickHudSrcCatchTicks;
volatile u32 gNdsTickHudSrcParamsTicks;
/* Cycle 92 SGCO split. The remaining three of the six per-fighter procs, which
 * cycle 86 could not bracket because they were ITCM-pinned by their decomp
 * symbol names. They are renamed now and linker/nds_hot_text.ld moved with them
 * in the same commit. Nested inside GCRA exactly like the four above, so they
 * are published after `named` is summed and never added to it. SCPU nests
 * inside SINT, so the analyzer derives SITR = SINT - SCPU rather than double
 * counting the AI. */
volatile u32 gNdsTickHudSrcInterruptTicks;
volatile u32 gNdsTickHudSrcPhysicsDefaultTicks;
volatile u32 gNdsTickHudSrcPhysicsCaptureTicks;
volatile u32 gNdsTickHudFlushTicks;
/* R2-07: MISC is DrawTicks minus (FTR + STG + BG + HUD) plus the flush, i.e.
 * everything drawn that no other bucket claims. It is now the campaign, and a
 * residual cannot be optimised -- so these split it into the sub-paths that
 * actually make it up.
 *
 * CUMULATIVE, deliberately, and NOT reset per frame like the buckets beside
 * them. The tick-HUD ring carries bucket words only, so a per-frame value can
 * only be read one frame per GDB stop; a running total differenced across two
 * ring stops gives the whole window instead, which is what -PerStopGlobals is
 * for. Ticks and not counts: the campaign has twice mistaken presence for cost
 * (particles at 0.21 quads/frame, and the six proof-scoped counters that read
 * zero for a whole match), and a count cannot tell those apart. */
volatile u32 gNdsMiscWeaponDrawTicks;
volatile u32 gNdsMiscEffectDrawTicks;
volatile u32 gNdsMiscParticleDrawTicks;
/* Written by ndsBattlePlayableFinalizePresentedIteration, which is what keeps
 * the three above from being collected: --gc-sections drops a volatile u32
 * that live code never names, and a debugger is not live code. This is also
 * the cross-check -- MISC minus this is the part of the bucket the split does
 * not yet explain. */
volatile u32 gNdsMiscSplitAccountedTicks;
/* R2-07 effect-cost probe. The MISC split put 359,717 ticks/frame in the
 * effect DObj submit on over-gate windows and 0 on clean ones, which is the
 * shape of something rebuilt per frame rather than of a merely generic walk.
 * 38bba475's own message flags the suspect: its G_CC_BLENDPE prim/env bake
 * "mints a distinct cached texture variant per distinct (prim,env) pair, so
 * any effect that ramps or fades its colour mints one per frame", and it says
 * that is unmeasured. Every effect in this set ramps or fades.
 *
 * Upload TICKS do not exist below NDS_RENDERER_PROFILE_LEVEL 1 and the tick-HUD
 * build is level 0, so the span is re-bracketed here rather than reused. Counts
 * and bytes do have a level-0 path but only into a per-frame struct that a ring
 * stop can sample one frame of; these are cumulative so a per-stop delta gives
 * the whole window. Evictions need no new counter --
 * sNdsRendererRuntimeTextureCacheEvictCount is already cumulative. */
volatile u32 gNdsMiscTexUploadTicks;
volatile u32 gNdsMiscTexUploadCount;
volatile u32 gNdsMiscTexUploadBytes;
/* R2-08 effect-submit PHASE SPLIT. The upload probe above refuted cache thrash
 * (1 upload in ~1,408 steady-state frames, 0.0071% of the effect cost) and left
 * 6,990 ticks per effect triangle unexplained, so this splits the submit itself.
 *
 * THE PHASE NAMES COME FROM THE CALL GRAPH, not from a guess: SubmitEffectDObjTree
 * -> SubmitStageDObjTreeDepth (recursive) -> SubmitStageDObjNode -> SubmitStageDL,
 * and SubmitStageDL does four separable things before it executes anything --
 * a loaded-file lookup, a material-segment synthesis, an initial-matrix build,
 * and then the generic interpreter. Two of those (Find, Material) are NOT in any
 * a-priori taxonomy of "walk / dispatch / matrix / texture / submit" and are
 * exactly why the graph was read first.
 *
 * NESTED SPANS, so they subtract rather than sum: Tree contains every DL, and DL
 * contains Find+Material+Matrix+Exec. Tree-DL is the walk; DL-(the four) is the
 * per-list overhead; Exec-Tex is vertex/primitive submission. The residual the
 * report owes is EffectDraw - Color - Tree, and it is reported even when zero. */
volatile u32 gNdsEffectPhaseColorTicks;
volatile u32 gNdsEffectPhaseTreeTicks;
volatile u32 gNdsEffectPhaseDLTicks;
volatile u32 gNdsEffectPhaseFindTicks;
volatile u32 gNdsEffectPhaseMaterialTicks;
volatile u32 gNdsEffectPhaseMatrixTicks;
volatile u32 gNdsEffectPhaseExecTicks;
volatile u32 gNdsEffectPhaseTexTicks;
volatile u32 gNdsEffectPhaseDLCount;
volatile u32 gNdsEffectPhaseNodeCount;
/* Set for the duration of the effect tree submit so nds_renderer.c can charge
 * texture resolves to the effect layer without knowing anything about it.
 * sNdsRendererAdapterEffectSubmitActive already exists but is file-static in
 * reloc_backend_renderer_dl.c; this is its cross-TU twin, set from the same
 * seam so the two cannot drift. */
volatile u32 gNdsEffectPhaseActive;
/* R2-08 cap-versus-end. Cumulative; see the publish site in
 * reloc_backend_renderer_dl.c for why a tautology made these necessary. */
volatile u32 gNdsEffectDLCommandTotal;
volatile u32 gNdsEffectDLTermCapCount;
volatile u32 gNdsEffectDLTermEndCount;
volatile u32 gNdsEffectDLTermOtherCount;
volatile u32 gNdsEffectDLTermOtherMask;
/* G3 step 0, the unique-template census. Cumulative from boot and deliberately
 * NOT reset beside the other effect counters: the arena the packet builder
 * needs must hold every template the match uses, so the wanted figure is the
 * running total at the last ring stop, not a per-frame value. */
volatile u32 gNdsEffectDLCensusUnique;
volatile u32 gNdsEffectDLCensusOverflow;
volatile u32 gNdsEffectDLCensusStateVariants;
volatile u32 gNdsEffectDLCensusCommandVariants;
volatile u32 gNdsEffectDLCensusCommandMax;
volatile u32 gNdsEffectDLCensusUniqueCommandTotal;
volatile u32 gNdsEffectDLTriangleTotal;
volatile u32 gNdsEffectDLVertexTotal;
volatile u32 gNdsEffectDLCensusGeomVariants;
volatile u32 gNdsEffectDLCensusTrisMaxTotal;
volatile u32 gNdsEffectDLCensusVertsMaxTotal;
/* G3 step 1, the effect GX stream verdict. The capture globals themselves live
 * in nds_renderer.c beside the GX funnel that fills them; these are the
 * per-template comparison, accumulated in reloc_backend_renderer_dl.c beside
 * the unique-template census whose key table it reuses. Cumulative from boot
 * for the same reason the census is: the question is about the whole match. */
volatile u32 gNdsEffectPacketTemplates;
volatile u32 gNdsEffectPacketTableOverflow;
volatile u32 gNdsEffectPacketGeomMatchCount;
volatile u32 gNdsEffectPacketGeomVariantCount;
volatile u32 gNdsEffectPacketColorMatchCount;
volatile u32 gNdsEffectPacketColorVariantCount;
volatile u32 gNdsEffectPacketMatrixMatchCount;
volatile u32 gNdsEffectPacketMatrixVariantCount;
volatile u32 gNdsEffectPacketGeomWordVariantCount;
/* G3 step 3, option B's price. Three measured spans nested inside the existing
 * Exec bracket, all armed by the same flag as the cycle-88 capture; traversal
 * is DERIVED as Exec - TexInExec - Vtx - Tri so that a negative residual would
 * disprove the nesting rather than pass silently. See the comment beside the
 * span helpers in nds_renderer.c for why the timer reads bias the answer toward
 * B being small. */
volatile u32 gNdsEffectPhaseVtxTicks;
volatile u32 gNdsEffectPhaseVtxCount;
volatile u32 gNdsEffectPhaseTriTicks;
volatile u32 gNdsEffectPhaseTriCount;
volatile u32 gNdsEffectPhaseTexInExecTicks;
/* G3 step 5: the painter depth-slot census. Cumulative maxima and sums over the
 * whole match, folded once per renderer-owned hardware frame. See the header for
 * why the band is 128 rather than the 4,096 the board inferred. */
volatile u32 gNdsPainterSlotFrames;
volatile u32 gNdsPainterSlotBgMax;
volatile u32 gNdsPainterSlotFgMax;
volatile u32 gNdsPainterSlotTotalMax;
volatile u32 gNdsPainterSlotBgSum;
volatile u32 gNdsPainterSlotFgSum;
volatile u32 gNdsPainterSlotBgOverBand;
volatile u32 gNdsPainterSlotFgOverBand;
/* Cycle 98: the FTR pre-submission census. See nds_startup.h for what each
 * group answers and why none of them is a timer. Cumulative from boot and
 * deliberately never reset -- the question is about the whole match, and a
 * per-frame value would have to be differenced across ring stops to answer it
 * anyway. */
volatile u32 gNdsFtrPreValidateReuse;
volatile u32 gNdsFtrPreValidateBuild;
volatile u32 gNdsFtrPreValidateReject;
volatile u32 gNdsFtrPreWalkSame;
volatile u32 gNdsFtrPreWalkVariant;
volatile u32 gNdsFtrPreWalkFirst;
volatile u32 gNdsFtrPreMatCalls;
volatile u32 gNdsFtrPreMatSame;
volatile u32 gNdsFtrPreMatVariant;
volatile u32 gNdsFtrPreMatNew;
volatile u32 gNdsFtrPreMatEvict;
volatile u32 gNdsFtrPreResetTransient;
volatile u32 gNdsFtrPreResetRuntime;
/* Task 66: the idle VBlank span, owned by the tick HUD rather than borrowed
 * from gNdsRendererProfileVBlankWaitTicks. That counter only accumulates under
 * NDS_RENDERER_PROFILE_LEVEL >= 1, and both the tick-HUD and proof targets pin
 * the profile level to 0 -- so the wait was measurable everywhere except in the
 * configuration every measurement is actually taken in. */
volatile u32 gNdsTickHudVBlankWaitTicks;
/* Task 67/68: how often the fighter draw abandons the native production owner
 * and falls through to the generic display-list interpreter, and at which of
 * the four rejection points. The per-PC census showed the P95 frames are not
 * doing more work -- they are interpreting instead of replaying, with ScanList
 * and SubmitVertex at ~10x their normal rate while animation joint playback
 * drops to nothing. Counting the fallback directly turns that from an
 * inference off two windowed censuses into a per-frame measurement.
 *
 * Cumulative; the sampler differences consecutive frames. Tick-HUD only, so
 * the published ROM is untouched. */
#if NDS_TASK68_FALLBACK_CENSUS
volatile u32 gNdsTickHudNativeOwnerFallbackCount;
volatile u32 gNdsTickHudNativeOwnerFallbackByReason[
    nNDSTickHudNativeOwnerFallbackReasonCount];
#endif
#if NDS_TASK75_LOAD_CENSUS
/* Task 75 E0. Task 71 profiled one expensive `SRC` frame and found a NitroFS
 * open, a cartridge read, a relocation and a figatree parse inside the frame
 * that needed the animation -- but section 5 left the obligation open: one
 * frame was profiled, and it was never shown that every high-`SRC` frame is a
 * load. Task 106 made that question the gate's, by proving the `SRC` excursion
 * survives halving the update rate unchanged (+518,016 vs +522,720).
 *
 * Counts completed file loads. Cumulative, differenced per frame into the same
 * census ring the Task 70 fallback counter uses, so the load count and the
 * bucket costs land on one index and can be compared directly rather than
 * correlated across separate runs. Tick-HUD lab builds only. */
volatile u32 gNdsTask75AssetLoadCount;
#endif
#endif
/* Cycle 99: the baked fighter draw plan. These sit OUTSIDE the NDS_TICK_HUD
 * block on purpose and the placement is load-bearing: gNdsFtrPlanRoute is the
 * shipped route selector, read by the draw itself, so defining it inside that
 * block broke the NDS_TICK_HUD=0 link with an undefined reference from
 * ndsFighterMarioFoxDLAllDrawForSlot. That is exactly what the proof-target
 * link check exists to catch -- it caught it here on the first try.
 *
 * The other five are written only from #if NDS_TICK_HUD code and referenced
 * only by the taskman_seam marker block, so --gc-sections drops them from the
 * shipped ROM; the shipped cost of this group is the four bytes of Route. */
/* Cycle 100: the route is selected at BUILD time, not poked, and it is pinned
 * to .data in both arms.
 *
 * The poke does not work here and the mechanism is measured, not guessed:
 * `-SetGlobals` writes main RAM through the GDB stub, and this flag shares its
 * 32-byte ARM9 D-cache line with gNdsTickHudVBlankWaitTicks, which the tick HUD
 * writes every frame. That line is permanently resident and dirty, so the guest
 * reads its stale cached 0 and every writeback stamps that 0 back over the
 * poke. Measured: poked 7, read back 7 in the same stop, 0 plan hits over 1,216
 * draws, 0 at end of run -- while gNdsFtrPlanVerify four bytes lower, in the
 * PREVIOUS cache line, survived the identical batch. There is no rogue store;
 * the disassembly's only three references to this address are loads.
 *
 * Pinning it to .data (rather than letting the 0 arm fall into .bss) is what
 * makes the A/B honest: both arms then place every section identically and
 * differ in exactly this one initialised word. */
volatile u32 gNdsFtrPlanRoute __attribute__((section(".data"))) =
    NDS_FTR_PLAN_ROUTE;
/* Same treatment, and here the cache-line argument is not merely a hazard but a
 * certainty: gNdsFtrPlanVerify shares its 32-byte line with gNdsFtrPlanHit,
 * which increments on every baked draw, so on the arm worth verifying the line
 * is always dirty and a poke is always stamped back to 0. */
volatile u32 gNdsFtrPlanVerify __attribute__((section(".data"))) =
    NDS_FTR_PLAN_VERIFY;
volatile u32 gNdsFtrPlanHit;
volatile u32 gNdsFtrPlanBuild;
volatile u32 gNdsFtrPlanVerifyRuns;
volatile u32 gNdsFtrPlanVerifyMismatch;
