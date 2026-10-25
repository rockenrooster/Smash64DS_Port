/* Diagnostic words, collision and the battle's harnesses: the collision
 * runtime record, the projectile and weapon proofs, the build and battle
 * words. Included in place by nds_startup.h (inside its guard); not a
 * header of its own. */

typedef struct NDSCollisionRuntimeDiagnostics {
    u32 floor_sweep_calls;
    u32 floor_sweep_hits;
    u32 floor_flat_ascending_sweeps;
    u32 floor_flat_ascending_accepts;
    u32 floor_reverse_endpoint_hits;
    u32 floor_adj_calls;
    u32 floor_adj_direct_hits;
    u32 floor_adjacent_hits;
    u32 floor_adj_pass_rejects;
    u32 floor_adj_ambiguous;
    u32 topology_build_attempts;
    u32 topology_build_successes;
    u32 topology_rebuilds;
    u32 topology_lines;
    u32 topology_shared_directed;
    u32 topology_orphan_endpoints;
    u32 topology_reversed_lines;
    u32 topology_ambiguous_endpoints;
    u32 topology_invalid;
    u32 topology_getter_invalid;
    u32 topology_getter_calls;
    u32 topology_getter_hits;
    u32 topology_getter_no_edge;
    u32 topology_inactive;
    u32 topology_hash;
    s32 topology_last_line;
    s32 topology_last_prev;
    s32 topology_last_next;
    u32 default_copy_calls;
    u32 default_run_calls;
    u32 default_reset_calls;
    u32 default_fighter_calls;
    u32 default_weapon_calls;
    u32 damage_check_calls;
    u32 damage_proc_calls;
    u32 damage_floor_tests;
    u32 damage_floor_hits;
    /* Knockback wall/ceiling contacts that ended the sub-step loop. Both read
     * zero for as long as those branches were deferred out of the damage
     * proc; a non-zero count is the only evidence that a knocked-back fighter
     * is being stopped by geometry rather than flying through it. */
    u32 damage_wall_hits;
    u32 damage_ceil_hits;
    u32 damage_floor_landings;
    u32 damage_floor_edge_deferred;
    u32 damage_results;
    u32 damage_invalid;
    s32 damage_last_line;
    s32 damage_last_status;
    s32 damage_last_root_y_before_milli;
    s32 damage_last_root_y_after_milli;
    s32 damage_last_pos_diff_y_milli;
    s32 damage_last_angle_y_milli;
    u32 damage_last_mask_curr;
    u32 damage_last_mask_stat;
    u32 fireball_map_calls;
    u32 fireball_floor_rises;
    u32 fireball_rebounds;
    u32 fireball_upward_floor_rises;
    u32 fireball_survives;
    u32 fireball_destroys;
    s32 fireball_last_line;
    u32 fireball_last_flags;
    u32 fireball_last_mask_prev;
    u32 fireball_last_mask_curr;
    s32 fireball_pre_vx_milli;
    s32 fireball_pre_vy_milli;
    s32 fireball_post_vx_milli;
    s32 fireball_post_vy_milli;
    s32 fireball_pre_speed_milli;
    s32 fireball_post_speed_milli;
    s32 fireball_angle_x_milli;
    s32 fireball_angle_y_milli;
    u32 fireball_last_result;
} NDSCollisionRuntimeDiagnostics;

extern volatile NDSCollisionRuntimeDiagnostics
    gNdsCollisionRuntimeDiagnostics;
void ndsCollisionRuntimeDiagnosticsReset(void);

extern volatile u32 gNdsFighterProjectileProofResult;
extern volatile u32 gNdsFighterProjectileProofMask;
extern volatile u32 gNdsFighterProjectileProofActorSlot;
extern volatile u32 gNdsFighterProjectileProofActorKind;
extern volatile u32 gNdsFighterProjectileProofBPressFrames;
extern volatile u32 gNdsFighterProjectileProofSpecialStatusFrames;
extern volatile u32 gNdsFighterProjectileProofSpecialMotion;
extern volatile u32 gNdsFighterProjectileProofAccessoryFrames;
extern volatile u32 gNdsFighterProjectileProofFlag0Frames;
extern volatile u32 gNdsFighterProjectileProofSpawnCallCount;
/* Set only around wpFoxBlasterMakeWeapon's source constructor, and only
 * when the shooter is Kirby: his joint 17 puts the muzzle flash on his own
 * body. Read by BOTH definitions of efManagerFoxBlasterGlowMakeEffect --
 * the weak one in battleship_fox_blaster.c and the AOT override in
 * battleship_efmanager.c, which is the one that links at
 * NDS_R2_FOX_BLASTER_GLOW_AOT=1. Impact glows are outside the window. */
extern sb32 gNdsFoxBlasterSuppressMuzzleGlow;
extern volatile u32 gNdsFighterDamageFireCallCount;
extern volatile u32 gNdsFighterEffectKindMask0;
extern volatile u32 gNdsFighterEffectKindMask1;
extern volatile u32 gNdsFighterEffectKindMask2;
extern volatile u32 gNdsFighterEffectKindMask3;
extern volatile u32 gNdsFighterModelPartSetCount;
extern volatile u32 gNdsFighterModelPartOnCount;
extern volatile u32 gNdsFighterModelPartResetCount;
extern volatile u32 gNdsFighterProjectileProofSpawnSuccessCount;
extern volatile u32 gNdsFighterProjectileProofUpdateDestroyCount;
extern volatile u32 gNdsFighterProjectileProofMapDestroyCount;
extern volatile u32 gNdsFighterProjectileProofHitDestroyCount;
extern volatile u32 gNdsFighterProjectileProofWeaponFrames;
extern volatile u32 gNdsFighterProjectileProofWeaponCountMax;
/* Which of wpManagerMakeWeapon's two NULL returns fired. */
extern volatile u32 gNdsFighterProjectileProofSpawnFailGObjCount;
extern volatile u32 gNdsFighterProjectileProofSpawnFailPoolCount;
/* sGCCommonsMaxNum / sGCCommonsActiveNum sampled AT the refusal. */
extern volatile u32 gNdsFighterProjectileProofSpawnFailGObjMax;
extern volatile u32 gNdsFighterProjectileProofSpawnFailGObjActive;
extern volatile u32 gNdsFighterProjectileProofSpawnFailHeapFree;
extern volatile u32 gNdsWeaponStructBytes;
extern volatile u32 gNdsWeaponPoolEntries;
extern volatile u32 gNdsWeaponPoolRefusalCount;
extern volatile u32 gNdsWeaponPoolLiveHighWater;
extern volatile u32 gNdsITCommonDataRejectCount;
/* Battle-time low-water of gSYTaskmanGeneralHeap. Under 25,600 means
 * ifCommonSetMaxNumGObj has capped the GObj pool for the rest of the match. */
extern volatile u32 gNdsTaskmanGeneralHeapFreeMin;
/* Peak live DObjs. x136 bytes is heap the match never gives back. */
extern volatile u32 gNdsGCDrawsActiveMax;
/* DS weapon-pool size, replacing the source's WEAPON_ALLOC_MAX 32. Ten is the
 * bounded census ceiling for the P2 four-CPU/item fidelity pass: the standing
 * 33,672-byte general-heap low-water can afford seven more 704-byte entries
 * than the old pool of three while remaining above the 25,600-byte floor. The
 * taskman arena is selected in 4,096-byte steps, so that subtraction is only a
 * pre-build bound; the real low-water must stay above the floor. The final
 * value is reduced to measured live high-water plus margin after both the
 * stress and projectile-heavy rosters are sampled. */
#ifndef NDS_R2_WEAPON_POOL
#define NDS_R2_WEAPON_POOL 10
#endif
/* The effect-instance pool is the same kind of DS pool-size override; it lives
 * in include/nds/nds_effects.h, beside the effect counters it is measured with. */
extern volatile u32 gNdsFighterProjectileProofKindMask;
extern volatile u32 gNdsFighterProjectileProofAttackStateMask;
extern volatile u32 gNdsFighterProjectileProofDamageMax;
extern volatile u32 gNdsFighterProjectileProofLifetimeMax;
extern volatile u32 gNdsFighterProjectileProofMapMask;
extern volatile s32 gNdsFighterProjectileProofFireballIndex;
extern volatile s32 gNdsFighterProjectileProofFireballInitialVelXMilli;
extern volatile s32 gNdsFighterProjectileProofFireballInitialVelYMilli;
extern volatile u32 gNdsFighterReflectorProofResult;
extern volatile u32 gNdsFighterReflectorProofMask;
extern volatile u32 gNdsFighterReflectorProofFoxSlot;
extern volatile u32 gNdsFighterReflectorProofProjectileSlot;
extern volatile u32 gNdsFighterReflectorProofDownBPressFrames;
extern volatile u32 gNdsFighterReflectorProofStartFrames;
extern volatile u32 gNdsFighterReflectorProofLoopFrames;
extern volatile u32 gNdsFighterReflectorProofHitFrames;
extern volatile u32 gNdsFighterReflectorProofIsReflectFrames;
extern volatile s32 gNdsFighterReflectorProofReflectLRBeforeHit;
extern volatile u32 gNdsFighterReflectorProofReflectLRClearFrames;
extern volatile u32 gNdsFighterReflectorProofHitSetCallCount;
extern volatile u32 gNdsFighterReflectorProofFireballProcCount;
extern volatile s32 gNdsFighterReflectorProofFireballVelXBefore;
extern volatile s32 gNdsFighterReflectorProofFireballVelXAfter;
extern volatile u32 gNdsFighterReflectorProofFireballOwnerKind;
extern volatile u32 gNdsFighterReflectorProofFireballCanReflect;
extern volatile u32 gNdsFighterReflectorProofFireballCanAbsorb;
extern volatile u32 gNdsFighterReflectorProofFireballCanShield;
extern volatile u32 gNdsFighterReflectorProofFireballAttackCount;
extern volatile u32 gNdsFighterReflectorProofFireballDamage;
extern volatile u32 gNdsFighterReflectorProofFireballSizeMilli;
extern volatile s32 gNdsFighterReflectorProofFireballDXMilli;
extern volatile s32 gNdsFighterReflectorProofFireballDYMilli;
extern volatile u32 gNdsFireballNativeMapCallCount;
extern volatile u32 gNdsFireballNativeMapHandledCount;
extern volatile u32 gNdsFireballNativeMapFallbackCount;
extern volatile u32 gNdsFireballNativeMapCollisionCount;
extern volatile u32 gNdsFireballNativeMapFloorHitCount;
extern volatile u32 gNdsFireballNativeMapCeilHitCount;
extern volatile u32 gNdsFireballNativeMapLWallHitCount;
extern volatile u32 gNdsFireballNativeMapRWallHitCount;
extern volatile u32 gNdsFireballNativeMapLastMask;
extern volatile s32 gNdsFireballNativeMapLastLineID;
extern volatile u32 gNdsFighterReflectorProofSpecialSizeMilli;
extern volatile u32 gNdsFighterReflectorProofSpecialResist;
extern volatile u32 gNdsFighterSpecialsProofMask;
extern volatile u32 gNdsFighterSpecialsProofPhase;
extern volatile u32 gNdsFighterSpecialsProofPhaseFrames;
extern volatile u32 gNdsFighterSpecialsMarioSlot;
extern volatile u32 gNdsFighterSpecialsFoxSlot;
extern volatile u32 gNdsFighterSpecialsMarioHiPressFrames;
extern volatile u32 gNdsFighterSpecialsMarioHiFrames;
extern volatile u32 gNdsFighterSpecialsMarioAirHiFrames;
extern volatile u32 gNdsFighterSpecialsMarioFallSpecialFrames;
extern volatile u32 gNdsFighterSpecialsMarioLandingFallSpecialFrames;
extern volatile u32 gNdsFighterSpecialsMarioHiWaitFrames;
extern volatile s32 gNdsFighterSpecialsMarioHiRootYMilli;
extern volatile u32 gNdsFighterSpecialsMarioHiDamageMax;
extern volatile u32 gNdsFighterSpecialsMarioLwPressFrames;
extern volatile u32 gNdsFighterSpecialsMarioLwFrames;
extern volatile u32 gNdsFighterSpecialsMarioAirLwFrames;
extern volatile u32 gNdsFighterSpecialsMarioLwDustEffectCount;
extern volatile u32 gNdsFighterSpecialsMarioLwWaitFrames;
extern volatile u32 gNdsFighterSpecialsMarioLwDamageMax;
extern volatile u32 gNdsFighterSpecialsFoxHiPressFrames;
extern volatile u32 gNdsFighterSpecialsFoxHiStartFrames;
extern volatile u32 gNdsFighterSpecialsFoxHiHoldFrames;
extern volatile u32 gNdsFighterSpecialsFoxHiTravelFrames;
extern volatile u32 gNdsFighterSpecialsFoxHiEndFrames;
extern volatile u32 gNdsFighterSpecialsFoxHiBoundFrames;
extern volatile u32 gNdsFighterSpecialsFoxHiWaitFrames;
extern volatile s32 gNdsFighterSpecialsFoxHiRootYMilli;
#if NDS_DONKEY
extern volatile u32 gNdsFighterDonkeySpecialsSlot;
extern volatile u32 gNdsFighterDonkeySpecialsNChargePressFrames;
extern volatile u32 gNdsFighterDonkeySpecialsNStartFrames;
extern volatile u32 gNdsFighterDonkeySpecialsNLoopFrames;
extern volatile u32 gNdsFighterDonkeySpecialsNStorePressFrames;
extern volatile u32 gNdsFighterDonkeySpecialsNStoredChargeMax;
extern volatile u32 gNdsFighterDonkeySpecialsNStoredWaitFrames;
extern volatile u32 gNdsFighterDonkeySpecialsNResumePressFrames;
extern volatile u32 gNdsFighterDonkeySpecialsNReleaseTapFrames;
extern volatile u32 gNdsFighterDonkeySpecialsNEndFrames;
extern volatile u32 gNdsFighterDonkeySpecialsNReleaseChargeMax;
extern volatile u32 gNdsFighterDonkeySpecialsNPassiveResetFrames;
extern volatile u32 gNdsFighterDonkeySpecialsNReleaseWaitFrames;
extern volatile u32 gNdsFighterDonkeySpecialsHiPressFrames;
extern volatile u32 gNdsFighterDonkeySpecialsHiFrames;
extern volatile u32 gNdsFighterDonkeySpecialsHiGroundGAFrames;
extern volatile u32 gNdsFighterDonkeySpecialsHiWaitFrames;
extern volatile u32 gNdsFighterDonkeySpecialsLwPressFrames;
extern volatile u32 gNdsFighterDonkeySpecialsLwStartFrames;
extern volatile u32 gNdsFighterDonkeySpecialsLwLoopFrames;
extern volatile u32 gNdsFighterDonkeySpecialsLwRepeatPressFrames;
extern volatile u32 gNdsFighterDonkeySpecialsLwLoopFlagFrames;
extern volatile u32 gNdsFighterDonkeySpecialsLwEndFrames;
extern volatile u32 gNdsFighterDonkeySpecialsLwWaitFrames;
#endif
extern volatile u32 gNdsFighterSamusSpecialsSlot;
extern volatile u32 gNdsFighterSamusSpecialsNPressFrames;
extern volatile u32 gNdsFighterSamusSpecialsNStartFrames;
extern volatile u32 gNdsFighterSamusSpecialsNLoopFrames;
extern volatile u32 gNdsFighterSamusSpecialsNChargeMax;
extern volatile u32 gNdsFighterSamusSpecialsNFullWaitFrames;
extern volatile u32 gNdsFighterSamusSpecialsNReleasePressFrames;
extern volatile u32 gNdsFighterSamusSpecialsNEndFrames;
extern volatile u32 gNdsFighterSamusSpecialsNReleaseWaitFrames;
extern volatile u32 gNdsFighterSamusSpecialsLwPressFrames;
extern volatile u32 gNdsFighterSamusSpecialsLwFrames;
extern volatile u32 gNdsFighterSamusSpecialsLwWaitFrames;
extern volatile u32 gNdsFighterNaturalMovesetMask;
extern volatile u32 gNdsFighterNaturalMovesetPhase;
extern volatile u32 gNdsFighterNaturalMovesetPhaseFrames;
extern volatile u32 gNdsFighterNaturalMovesetTiltS3Frames;
extern volatile u32 gNdsFighterNaturalMovesetTiltHi3Frames;
extern volatile u32 gNdsFighterNaturalMovesetTiltLw3Frames;
extern volatile u32 gNdsFighterNaturalMovesetTiltHitboxFrames;
extern volatile u32 gNdsFighterNaturalMovesetSmashFrames;
extern volatile u32 gNdsFighterNaturalMovesetSmashHitboxFrames;
extern volatile u32 gNdsFighterNaturalMovesetAerialFrames;
extern volatile u32 gNdsFighterNaturalMovesetAerialHitboxFrames;
extern volatile u32 gNdsFighterNaturalMovesetLandingFrames;
extern volatile u32 gNdsFighterNaturalMovesetCatchFrames;
extern volatile u32 gNdsFighterNaturalMovesetCatchWaitFrames;
extern volatile u32 gNdsFighterNaturalMovesetThrowFrames;
extern volatile u32 gNdsFighterNaturalMovesetThrownFrames;
extern volatile u32 gNdsFighterNaturalMovesetThrowRecoverFrames;
extern volatile u32 gNdsFighterNaturalMovesetThrowDamageBefore;
extern volatile u32 gNdsFighterNaturalMovesetThrowDamageAfter;
extern volatile u32 gNdsFighterNaturalMovesetAttackerStatus;
extern volatile u32 gNdsFighterNaturalMovesetAttackerMotion;
extern volatile u32 gNdsFighterNaturalMovesetAttackerGA;
extern volatile s32 gNdsFighterNaturalMovesetAttackerRootYMilli;
extern volatile u32 gNdsFighterNaturalMovesetVictimStatus;
extern volatile u32 gNdsFighterNaturalMovesetVictimMotion;
extern volatile u32 gNdsFighterNaturalMovesetVictimGA;
extern volatile s32 gNdsFighterNaturalMovesetVictimRootYMilli;
extern volatile u32 gNdsFighterBattlePlayableResult;
extern volatile u32 gNdsFighterBattlePlayableMask;
extern volatile u32 gNdsFighterBattlePlayableVictimSlot;
extern volatile u32 gNdsFighterBattlePlayableVictimStockStart;
extern volatile u32 gNdsFighterBattlePlayableVictimStockFinal;
extern volatile u32 gNdsFighterBattlePlayableBattleStockStart;
extern volatile u32 gNdsFighterBattlePlayableBattleStockFinal;
extern volatile u32 gNdsFighterBattlePlayableFallsStart;
extern volatile u32 gNdsFighterBattlePlayableFallsFinal;
extern volatile u32 gNdsFighterBattlePlayableDeadFrames;
extern volatile u32 gNdsFighterBattlePlayableRebirthDownFrames;
extern volatile u32 gNdsFighterBattlePlayableRebirthStandFrames;
extern volatile u32 gNdsFighterBattlePlayableRebirthWaitFrames;
extern volatile u32 gNdsFighterBattlePlayableFallAfterRebirthFrames;
extern volatile u32 gNdsFighterBattlePlayableWaitAfterRebirthFrames;
extern volatile u32 gNdsFighterBattlePlayableFinalStatus;
extern volatile u32 gNdsFighterBattlePlayableFinalGA;
extern volatile u32 gNdsFighterBattlePlayableFinalFloor;
extern volatile u32 gNdsFighterBattlePlayableFinalIsRebirth;
extern volatile u32 gNdsFighterBattlePlayableFinalIsGhost;
extern volatile u32 gNdsFighterBattlePlayableFinalCameraMode;
extern volatile u32 gNdsFighterBattlePlayableKOStickFrames;
extern volatile u32 gNdsFighterBattlePlayableMapCallCount;
extern volatile u32 gNdsFighterBattlePlayableMapHitCount;
extern volatile u32 gNdsFighterBattlePlayableMapFloorHitCount;
extern volatile u32 gNdsFighterBattlePlayableMapCliffHitCount;
extern volatile u32 gNdsFighterBattlePlayableMapCeilHitCount;
extern volatile u32 gNdsFighterBattlePlayableMapLastMaskStat;
extern volatile u32 gNdsFighterBattlePlayableMapLastMaskCurr;
extern volatile s32 gNdsFighterBattlePlayableFinalXMilli;
extern volatile s32 gNdsFighterBattlePlayableFinalYMilli;
extern volatile s32 gNdsFighterBattlePlayableFinalVelXMilli;
extern volatile s32 gNdsFighterBattlePlayableFinalVelYMilli;
extern volatile s32 gNdsFighterBattlePlayableFinalFloorDistMilli;
extern volatile u32 gNdsFTComputerSetupCount;
extern volatile u32 gNdsFTComputerDamageDetectCount;
extern volatile u32 gNdsFTComputerProcessCount;
extern volatile u32 gNdsFTComputerTargetFrames;
extern volatile u32 gNdsFTComputerObjectiveMask;
extern volatile u32 gNdsFTComputerBehaviorMask;
extern volatile u32 gNdsFTComputerInputChangeCount;
extern volatile u32 gNdsFTComputerStickFrames;
extern volatile u32 gNdsFTComputerButtonAFrames;
extern volatile u32 gNdsFTComputerButtonBFrames;
extern volatile u32 gNdsFTComputerButtonZFrames;
extern volatile u32 gNdsFTComputerAttackFrames;
extern volatile u32 gNdsFTComputerHitboxFrames;
extern volatile u32 gNdsFTComputerGuardFrames;
extern volatile u32 gNdsFTComputerRecoveryFrames;
extern volatile u32 gNdsFTComputerStatusChangeCount;
extern volatile u32 gNdsFTComputerFinalStatus;
extern volatile u32 gNdsFTComputerFinalGA;
extern volatile u32 gNdsFTComputerFinalInputKind;
extern volatile u32 gNdsFTComputerMarioDamageMax;
extern volatile u32 gNdsFTComputerFloorLineCount;
extern volatile s32 gNdsFTComputerStartXMilli;
extern volatile s32 gNdsFTComputerMinXMilli;
extern volatile s32 gNdsFTComputerMaxXMilli;
extern volatile s32 gNdsFTComputerFinalXMilli;
extern volatile u32 gNdsBattlePlayablePacingResult;
extern volatile u32 gNdsBattlePlayablePacingMode;
extern volatile u32 gNdsBattlePlayablePacingLogicFrames;
extern volatile u32 gNdsBattlePlayablePacingUpdateBase;
extern volatile u32 gNdsBattlePlayablePacingPresentedFrames;
extern volatile u32 gNdsBattlePlayablePacingDrawCalls;
extern volatile u32 gNdsBattlePlayablePacingTimerTicks;
extern volatile u32 gNdsBattlePlayablePacingPresentFpsX10;
extern volatile u32 gNdsBattlePlayablePacingLogicFpsX10;
extern volatile u32 gNdsBattlePlayablePacingVBlankStart;
extern volatile u32 gNdsBattlePlayablePacingVBlanks;
extern volatile u32 gNdsBattlePlayablePacingRestartRequested;
extern volatile u32 gNdsBattlePlayablePacingPresentIntervalMin;
extern volatile u32 gNdsBattlePlayablePacingPresentIntervalMax;
extern volatile u32 gNdsBattlePlayablePacingPresentIntervalBucket[
    NDS_BATTLE_PLAYABLE_PACING_INTERVAL_BUCKET_COUNT];
extern volatile u32 gNdsBattlePlayablePacingCadenceViolationCount;
extern volatile u32 gNdsBattlePlayablePacingPhasePresentCount[
    NDS_BATTLE_PLAYABLE_PACING_PHASE_COUNT];
extern volatile u32 gNdsBattlePlayablePacingPhaseSlipCount[
    NDS_BATTLE_PLAYABLE_PACING_PHASE_COUNT];
/* The BPLAY_PACE marker's group, in marker field order (the array members
 * cover five fields each). Every object the realtime harness reads in one
 * BPLAY_PACE printf is here, because the harness cross-checks its members
 * against each other -- logicLag, drawLead and phaseLag are all differences
 * between two of them, so a torn read invents a tuple the guest cannot be in.
 * See NDS_PUBLISH_DEBUGGER_GROUP in nds_platform.h for why that happens and
 * for the rule this list exists to enforce. gNdsBattlePlayablePacingVBlankStart
 * and ...RestartRequested are deliberately absent: they are not in the marker.
 * check-gbi-decode-fixtures.ps1 pins this list against the harness's printf. */
#define NDS_BATTLE_PLAYABLE_PACING_GROUP(X) \
    X(gNdsBattlePlayablePacingResult) \
    X(gNdsBattlePlayablePacingMode) \
    X(gNdsBattlePlayablePacingLogicFrames) \
    X(gNdsBattlePlayablePacingPresentedFrames) \
    X(gNdsBattlePlayablePacingDrawCalls) \
    X(gNdsBattlePlayablePacingTimerTicks) \
    X(gNdsBattlePlayablePacingPresentFpsX10) \
    X(gNdsBattlePlayablePacingLogicFpsX10) \
    X(gNdsBattlePlayablePacingVBlanks) \
    X(gNdsBattlePlayablePacingPresentIntervalMin) \
    X(gNdsBattlePlayablePacingPresentIntervalMax) \
    X(gNdsBattlePlayablePacingCadenceViolationCount) \
    X(gNdsBattlePlayablePacingPhasePresentCount) \
    X(gNdsBattlePlayablePacingPhaseSlipCount)
/* The present-interval histogram's own group, published at the same seam but
 * kept OUT of the list above, because that list is pinned bilaterally against
 * the BPLAY_PACE printf and this array is not in that marker -- its readers are
 * probe-present-cadence.ps1, sample-tick-hud-buckets.ps1,
 * probe-battlepack-pacing.ps1 and ab-fireball-quad.ps1, all of which read it at
 * a gdb stop.
 *
 * IT WAS PUBLISHED BY ACCIDENT UNTIL 2026-08-16. DC_FlushRange cleans whole
 * 32-byte lines, and two members of the group above happened to share this
 * array's two lines -- CadenceViolationCount covering buckets 0-2 and
 * IntervalMax/Min covering buckets 3-5 -- so any relayout of diagnostics.c
 * would have silently broken the ONE histogram AGENTS.md requires every device
 * A/B report to carry, and broken it as stale numbers rather than as an error.
 * probe-present-cadence.ps1 recorded the hazard; this is the fix. */
#define NDS_BATTLE_PLAYABLE_PACING_HISTOGRAM_GROUP(X) \
    X(gNdsBattlePlayablePacingPresentIntervalBucket)
extern volatile u32 gNdsBuildModeCanonicalWord;
extern volatile u32 gNdsBuildModeShippedWord;
extern volatile u32 gNdsBuildModeFastWord;
extern volatile u32 gNdsSObjWallpaperCacheBuildCount;
extern volatile u32 gNdsSObjWallpaperCacheHitCount;
extern volatile u32 gNdsSObjWallpaperCacheFastDrawCount;
extern volatile u32 gNdsSObjWallpaperCacheFallbackCount;
extern volatile u32 gNdsSObjWallpaperCacheWidth;
extern volatile u32 gNdsSObjWallpaperCacheHeight;
extern volatile u32 gNdsSObjWallpaperCacheOpaquePixels;
extern volatile u32 gNdsSObjWallpaperCacheBuildTicks;
extern volatile u32 gNdsSObjWallpaperCacheDrawTicks;
extern volatile u32 gNdsSObjWallpaperFinalDirectCount;
extern volatile u32 gNdsSObjWallpaperFinalSkipCount;
extern volatile u32 gNdsSObjWallpaperFinalKeyChangeCount;
extern volatile u32 gNdsSObjWallpaperFinalPixelWriteCount;
extern volatile u32 gNdsSObjBackgroundStagingClearBytes;
extern volatile u32 gNdsSObjForegroundStagingClearBytes;
/* R2-07 R4b foreground layer memo. Skip+Redraw sum to the foreground layers a
 * run built; Overflow counts frames that exceeded the defer buffer and fell
 * back to immediate drawing. A healthy settled Results scene is nearly all
 * Skip; any Overflow at all means the buffer is undersized for the scene. */
extern volatile u32 gNdsSObjLayerMemoSkipCount;
extern volatile u32 gNdsSObjLayerMemoRedrawCount;
extern volatile u32 gNdsSObjLayerMemoOverflowCount;
/* R2-07 VS Results presented-frame cadence. Index n counts intervals of exactly
 * n VBlanks; index 15 is the 15-or-more bin. Sixteen bins because the owner set
 * P95 as this scene's acceptance metric and a percentile cannot be read out of
 * a distribution with a 5+ catch-all tail. Report P95 and the max, never a min
 * FPS or a half-second average. Observation only; the battle scheduler owns
 * pacing. */
extern volatile u32 gNdsVSResultsPresentIntervalBucket[16];
extern volatile u32 gNdsVSResultsPresentIntervalMax;
extern volatile u32 gNdsVSResultsPresentIntervalSamples;
extern volatile u32 gNdsRendererProfileFrameCount;
extern volatile u32 gNdsRendererProfileLevel;
extern volatile u32 gNdsRendererM2DetailedLedger;
extern volatile u32 gNdsRendererProfileUpdateTicks;
extern volatile u32 gNdsRendererProfilePresentTicks;
extern volatile u32 gNdsRendererProfileDrawTicks;
extern volatile u32 gNdsRendererProfileHudTicks;
