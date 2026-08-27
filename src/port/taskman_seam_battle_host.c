/* Effect-instance pool free count (efmanager.c:1720), sampled per presented
 * frame for the NDS_R2_EFFECT_POOL low-water. See include/nds/nds_effects.h. */
extern s32 sEFManagerStructsFreeNum;
extern volatile u32 gNdsEffectPoolFreeMin;
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
/* Task 106 E0 sizing lever. `SRC` is exactly this many logical updates per
 * presented frame -- `ndsRunMarioFoxProofUpdate` is the only writer of
 * `gNdsTickHudSourceTicks`, and it brackets `scVSBattleFuncUpdate`. Building
 * with 1 prices what a 30 Hz simulation would save, which `PROJECT_GOAL.md`
 * ranks third in the sacrifice order, above gameplay fidelity and frame rate.
 *
 * A build with 1 is NOT a candidate: it is uncompensated, so the match advances
 * one logical tick per present and plays at half speed. It measures the ceiling
 * and nothing else. Compensating it -- advancing timers, physics integration
 * and animation by two frames per tick -- is the real work, and is the part
 * that needs the owner's "substantially the same gameplay experience" call. */
#ifndef NDS_TASK106_UPDATES_PER_PRESENT
#define NDS_TASK106_UPDATES_PER_PRESENT 2u
#endif
#define NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT \
    NDS_TASK106_UPDATES_PER_PRESENT
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
#if NDS_TICK_HUD
static u32 sNdsBattlePlayableTickHudLoopStartTick;
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
    gNdsRendererPhase05WallpaperFullRowCount = 0u;
    gNdsRendererPhase05WallpaperIncrementalRowCount = 0u;
    gNdsRendererPhase05WallpaperChangedXCount = 0u;
    gNdsRendererPhase05WallpaperChangedRunCount = 0u;
    gNdsRendererPhase05WallpaperLongestChangedRun = 0u;
    gNdsRendererPhase05WallpaperRunGE2Count = 0u;
    gNdsRendererPhase05WallpaperRunGE2Pixels = 0u;
    gNdsRendererPhase05WallpaperRunGE4Count = 0u;
    gNdsRendererPhase05WallpaperRunGE4Pixels = 0u;
    gNdsRendererPhase05WallpaperRunGE8Count = 0u;
    gNdsRendererPhase05WallpaperRunGE8Pixels = 0u;
    gNdsRendererPhase05WallpaperScalarStoreCount = 0u;
    gNdsRendererPhase05WallpaperPackedStoreCount = 0u;
    gNdsRendererPhase05WallpaperDmaPixelCount = 0u;
    gNdsRendererPhase05WallpaperCopyPixelCount = 0u;

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

/* Defined next to the rest of the R2-07 R1 telemetry in
 * `src/import/battleship_mnvsresults.c`, which is where the span is closed. */
extern volatile u32 gNdsVSResultsTransitionStartTick;
extern volatile u32 gNdsVSResultsTransitionTicks;

/* Whether this seam runs the source controller read/update pair itself.
 *
 * NOW 0, and the earlier REFUTATION of this flag is WITHDRAWN. That test ran
 * before the publish interlock existed, so with this pair removed the surviving
 * double publishes still zeroed the tap and a working arm read as a dead one.
 *
 * What settled it was reading the linked binary instead of the sources.
 * `syMainThread5` builds and starts `syControllerThreadMain` through
 * `osCreateThread`/`osStartThread`, and `syControllerFuncRead` sits in roughly
 * ten taskman setup tables -- every scene's `func_controller`, Results included.
 * Both are reached ONLY through function pointers, never a direct call, so a
 * source grep reports them dead while the binary shows them live. Believing the
 * grep is what made every writer of `gSYControllerDevices` look innocent.
 *
 * With this pair at 1 the port and the source both drive the pipeline, and the
 * source always publishes last:
 *   1. seam reads     -- the rising edge lands in `unk04`
 *   2. seam publishes -- `button_tap` = 0x1000, `unk04` drained to 0
 *   3. `task_update` calls `func_controller` -> `syControllerFuncRead`, whose
 *      UPDATE event finds `sSYControllerIsUpdateData` already FALSE and parks
 *      itself in `sSYControllerWaitUpdate`
 *   4. on retrace the thread reaches controller.c:484, `else
 *      syControllerUpdateGlobalData();` -- raw and unconditional -- republishing
 *      `button_tap` = `unk04` = 0 while `button_hold` = `unk00` keeps 0x1000
 * Hence a tap that died with the hold intact, and an `InputSeenMask` of exactly
 * 0x1000 and never garbage: nothing was corrupting memory, a legitimate publish
 * was draining an empty accumulator. The interlock in
 * `src/import/battleship_sys_controller.c` cannot reach step 4 -- it renames the
 * decomp symbol at include time, so it guards the port's nine direct callers but
 * not the decomp's own three internal call sites. Stop competing instead: let
 * the source read and publish once per frame, as it does on the original. */
#ifndef NDS_SEAM_CONTROLLER_PAIR
#define NDS_SEAM_CONTROLLER_PAIR 0
#endif

static void ndsBattlePlayableRecordLifecycleTaskmanExit(void)
{
    /* R2-07 R1. Opens the Battle -> Results transition bracket that
     * `ndsMNVSResultsRecordFrame` closes on its first tick. This is the last
     * point the battle owns, so the span is exactly the dead air the owner sees
     * with the final battle frame still on screen. Re-armed on every exit, so a
     * later exit simply moves the start later, which is the wanted behaviour. */
    gNdsVSResultsTransitionStartTick = cpuGetTiming();
    gNdsVSResultsTransitionTicks = 0u;
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
#if NDS_SHIP_TELEMETRY || NDS_TICK_HUD || \
    (NDS_RENDERER_PROFILE_LEVEL >= 1)
    u32 start = cpuGetTiming();
#endif
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    u32 phase_start = start;
#endif

#if NDS_TASK9_FLOAT_CENSUS
    ndsTask9FloatCensusBeginUpdate();
#endif
    ndsTask39EffectsUpdate();
    scVSBattleFuncUpdate();
#if NDS_TASK9_FLOAT_CENSUS
    ndsTask9FloatCensusEndUpdate();
#endif
#if NDS_TASK9_STATE_HASH
    ndsTask9StateHashRecordUpdate();
#endif
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    {
        u32 phase_ticks = cpuGetTiming() - phase_start;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileSourceUpdateTicks = phase_ticks;
#endif
#if NDS_TICK_HUD
        gNdsTickHudSourceTicks += phase_ticks;
#endif
    }
    phase_start = cpuGetTiming();
#endif
    ndsAudioBackendUpdate();
#if NDS_TICK_HUD || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    {
        u32 phase_ticks = cpuGetTiming() - phase_start;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
        gNdsRendererProfileAudioUpdateTicks = phase_ticks;
#endif
#if NDS_TICK_HUD
        gNdsTickHudAudioTicks += phase_ticks;
#endif
    }
#endif
#if NDS_SHIP_TELEMETRY || NDS_TICK_HUD || \
    (NDS_RENDERER_PROFILE_LEVEL >= 1)
    gNdsRendererProfileUpdateTicks = cpuGetTiming() - start;
#endif
    dSYTaskmanUpdateCount++;
#if NDS_SHIP_TELEMETRY
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
#else
    (void)counter;
#endif
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
#if NDS_TICK_HUD
    ndsPlatformTickHudReset();
#endif
    gNdsBattlePlayablePacingDrawCalls = 0;
    gNdsBattlePlayablePacingTimerTicks = 0;
    gNdsBattlePlayablePacingPresentFpsX10 = 0;
    gNdsBattlePlayablePacingLogicFpsX10 = 0;
    gNdsBattlePlayablePacingVBlankStart = vblank;
    gNdsBattlePlayablePacingVBlanks = 0u;
    gNdsBattlePlayablePacingPresentIntervalMin = 0xffffffffu;
    gNdsBattlePlayablePacingPresentIntervalMax = 0u;
    for (phase = 0u;
         phase < NDS_BATTLE_PLAYABLE_PACING_INTERVAL_BUCKET_COUNT;
         phase++)
    {
        gNdsBattlePlayablePacingPresentIntervalBucket[phase] = 0u;
    }
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

#if NDS_P2_FOUR_CPU_STRESS && NDS_TICK_HUD
/* The four-CPU whole-match tick sampler consumes the in-guest 128-entry ring,
 * so the debugger only needs to wake often enough to drain that ring before it
 * wraps. Breaking on FrameComplete every presented frame makes GDB dominate
 * the measurement run. Keep that universal marker unchanged for all existing
 * harnesses, and expose a stress-only drain point every 32 presents instead.
 * 32 divides the sampler's 96-frame stop stride, leaves 4x ring-wrap margin,
 * and is compiled out of shipping and the non-stress Boundary regression
 * configurations. The P2-2 four-CPU Boundary arm is its only gate owner. */
void __attribute__((noinline, used)) ndsBattlePlayableTickHudSparseMarker(void)
{
    __asm__ volatile("" ::: "memory");
}
#endif

static void ndsBattlePlayablePresentFrame(void)
{
#if NDS_SHIP_TELEMETRY || NDS_TICK_HUD || \
    (NDS_RENDERER_PROFILE_LEVEL >= 1)
    u32 start = cpuGetTiming();
    u32 draw_start;
    u32 hud_start;
#endif
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
#if NDS_SHIP_TELEMETRY || (NDS_RENDERER_PROFILE_LEVEL >= 1)
#if NDS_RENDER_ECONOMY
    ndsRendererProfileFrameBegin(
        ((gSCManagerBattleState != NULL) &&
         (gSCManagerBattleState->game_status == nSCBattleGameStatusGo)) ?
            1u : 0u);
#else
    ndsRendererProfileFrameBegin();
#endif
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileBeginFrameTicks = 0u;
    gNdsRendererProfileWallpaperTicks = 0u;
    gNdsRendererProfileForegroundTicks = 0u;
    gNdsRendererProfileStageLayer0Ticks = 0u;
    gNdsRendererProfileFlushTicks = 0u;
    gNdsRendererProfileVBlankWaitTicks = 0u;
    gNdsRendererProfilePostVBlankTicks = 0u;
    gNdsRendererProfileThreadTicks = 0u;
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
    /* REWIND THE SOURCE DISPLAY-LIST HEADS FOR THIS FRAME. This is a freeze fix,
     * not hygiene, and it is the source's own per-frame reset -- taskman.c
     * :1093-1100 runs it before every scene draw, and the Results loop below
     * (:7040) already does. The battle loop never did, so `gSYTaskmanDLHeads`
     * advanced monotonically for the whole session while `sSYTaskmanDLBuffers`
     * stayed 60 KiB, and BattleShip catches that itself:
     * `syTaskmanCheckBufferLengths` (decomp sys/taskman.c:329) ends in
     * `while (TRUE);`, so the overrun presents as a total freeze with a moving
     * picture one moment and a dead one the next.
     *
     * Measured 2026-07-31, reproduced twice on a START rematch and confirmed by
     * the owner ("froze at 52 secs left again on the 2nd match"): PC spinning at
     * taskman.c:338, kind 0, `used=61488` against `len=61440` -- 48 bytes over,
     * with head 0 running into buffer 1's allocation, `MALLOCOVF=0` and the
     * graphics heap at `used=0`. One match's emission very nearly fits, which is
     * exactly why match one always survived and match two died a few seconds in.
     *
     * `ndsFighterDisplayContractCapture` (reloc_backend_renderer_dl.c:10998)
     * already knew: it saves the heads AND the graphics-heap pointer around each
     * fighter's source draw and restores them after. That fixed the fighter path
     * locally and left every other source draw -- stage, sprites, camera,
     * effects -- accumulating.
     *
     * The graphics heap deliberately does NOT get a reset here: it measured
     * `used=0` at the freeze, so nothing accumulates in it (the fighter contract
     * owns its only consumer and rewinds it itself), and rewinding an arena the
     * port may hold pointers into would be new risk bought for no measured
     * need. Add it only if a capture ever shows taskman.c:344 instead of :338.
     *
     * 2026-08-02: A CAPTURE SHOWED taskman.c:344, so the reset is here now, and
     * this is the owner's "hitting Fox's shield freezes match sometimes".
     *
     * `used=0` was true of the frame that was sampled and false of the frame the
     * freeze lands on -- the graphics heap leaks SIXTEEN BYTES A FRAME on this
     * path, and the two captures prove it is a leak rather than a high-water:
     *
     *   graphics heap 53,248 -> spun at used=53,264 after 3,328 presented frames
     *   graphics heap 81,920 -> spun at used=81,936 after 5,120 presented frames
     *
     * Same +16 overshoot at both sizes, and 81,920/53,248 = 5,120/3,328 to three
     * decimal places. 53,248/3,328 is exactly 16 bytes per frame, which is one
     * `Light` (ftdisplaylights.c:26 bumps the heap by one and nothing gives it
     * back) or one `Vtx` -- and `scVSBattleFuncLights` runs once per frame.
     * Growing the heap only buys frames, which is why the re-budget in
     * battleship_scvsbattle.c moved the freeze without removing it.
     *
     * The other frame loop in this file has always done it (see the
     * `syTaskmanResetGraphicsHeap` beside `func_80004AB0` in the fast-verify
     * block), citing taskman.c:1093-1100 -- "resets these arenas before every
     * source scene draw", both arenas, which is the source contract this path
     * was honouring by half. `ndsFighterDisplayContractCapture` saving and
     * restoring the pointer around the fighter draw is what made the leak
     * invisible: it hides the fighter's own consumption, so what accumulates is
     * everything else, at a rate small enough to take minutes.
     *
     * The "pointers into the arena" risk is why the reset goes HERE, at the top
     * of the present, in the same position and order the other loop uses: the
     * arena's contents are per-frame display data that has already been consumed
     * by the previous frame's flush. */
#if NDS_R2_COLLISION_L7_ORACLE
    /* R2-07 L7 step one, sampled HERE and not later: the invert latches the
     * oracle keys on are set by this frame's hit detection and knocked down by
     * ndsFTParamsInvalidateFighterParts as the NEXT tick moves joints, so this
     * is the last point at which "the joints collision inverted this frame" is
     * still readable. Read-only; off in both shipped blocks. */
    ndsR2CollisionOracleSampleFrame();
#endif
    /* P2-3r13: last point at which this frame's graphics-heap peak is readable
     * for everything the fighter draw did not roll back. */
    ndsTaskmanSampleGraphicsHeap();
    syTaskmanResetGraphicsHeap();
    func_80004AB0();
    ndsPlatformBeginFrame();
    ndsSObjPreviewBeginFrame();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileBeginFrameTicks = cpuGetTiming() - phase_start;
#endif
#if NDS_SHIP_TELEMETRY || NDS_TICK_HUD || \
    (NDS_RENDERER_PROFILE_LEVEL >= 1)
    draw_start = cpuGetTiming();
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsFighterMarioFoxStageGCDrawAllLoopPresentHardwareFrame();
#else
    gcDrawAll();
#endif
    ndsSObjPreviewEndFrame();
#if NDS_SHIP_TELEMETRY || NDS_TICK_HUD || \
    (NDS_RENDERER_PROFILE_LEVEL >= 1)
    gNdsRendererProfileDrawTicks = cpuGetTiming() - draw_start;
#endif
    gNdsBattlePlayablePacingDrawCalls++;
#if NDS_SHIP_TELEMETRY || NDS_TICK_HUD || \
    (NDS_RENDERER_PROFILE_LEVEL >= 1)
    hud_start = cpuGetTiming();
#endif
    ndsPlatformRenderDebugHud();
#if NDS_SHIP_TELEMETRY || NDS_TICK_HUD || \
    (NDS_RENDERER_PROFILE_LEVEL >= 1)
    gNdsRendererProfileHudTicks = cpuGetTiming() - hud_start;
#endif
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
    /* The battle-time low-water of the taskman general heap, which is the
     * number every memory decision on this target turns on:
     * ifCommonSetMaxNumGObj caps the GObj pool the instant it drops under
     * 25,600 and never lifts the cap, and until 2026-08-01 nothing sampled it
     * during a match -- the soak's end-of-run read is the Results scene, where
     * the heap has already been rewound. Two subtractions per presented
     * frame. */
    {
        u32 free_now = (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
                             (uintptr_t)gSYTaskmanGeneralHeap.ptr);

        if (free_now < gNdsTaskmanGeneralHeapFreeMin)
        {
            gNdsTaskmanGeneralHeapFreeMin = free_now;
        }
    }
    /* The DObj high-water, which is the other half of the same budget and had
     * no instrument until 2026-08-01. gcGetDObjSetNextAlloc (objman.c:692)
     * grows the pool out of gSYTaskmanGeneralHeap 136 bytes at a time and --
     * unlike GObjs, which ifCommonSetMaxNumGObj caps -- has NO ceiling, so a
     * DObj peak is heap claimed for the rest of the match. Routing an effect to
     * its source implementation trades a shared template for a real DObj tree,
     * and this is what prices that trade: peak x 136 bytes against the margin
     * between the low-water above and the 25,600 latch. */
    if ((u32)sGCDrawsActiveNum > gNdsGCDrawsActiveMax)
    {
        gNdsGCDrawsActiveMax = (u32)sGCDrawsActiveNum;
    }
    /* Saturation of the NDS_R2_EFFECT_POOL bound. This is what sizes the pool
     * from measurement instead of a guess: the depth is right when the
     * low-water sits just above the source's 5-free refusal cut during heavy
     * combat, because that is the smallest pool that never refuses a cosmetic
     * effect the player would have seen. */
    if ((sEFManagerStructsFreeNum >= 0) &&
        ((u32)sEFManagerStructsFreeNum < gNdsEffectPoolFreeMin))
    {
        gNdsEffectPoolFreeMin = (u32)sEFManagerStructsFreeNum;
    }
#if NDS_RENDERER_M3_PHASE0_PROFILE
    NDS_RENDERER_PHASE05_FINISH(
        gNdsRendererPhase05PresentTailTicks, phase05_start);
#endif
#if NDS_SHIP_TELEMETRY || NDS_TICK_HUD || \
    (NDS_RENDERER_PROFILE_LEVEL >= 1)
    gNdsRendererProfilePresentTicks = cpuGetTiming() - start;
#endif
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
#if NDS_SHIP_TELEMETRY || (NDS_RENDERER_PROFILE_LEVEL >= 1)
    ndsRendererProfileFramePublish();
#endif
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
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfilePresentIntervalVBlanks = interval;
#endif
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
    /* Bucket the interval into 2/3/4/(5+) for the device A/B histogram. The
     * locked-2-present scheduler makes 2 the floor, so anything below is
     * defensive; anything 5 or above collapses into the 5+ bucket. */
    {
        u32 bucket = (interval < NDS_BATTLE_PLAYABLE_PACING_INTERVAL_BUCKET_5PLUS)
            ? interval
            : NDS_BATTLE_PLAYABLE_PACING_INTERVAL_BUCKET_5PLUS;

        if (bucket >= NDS_BATTLE_PLAYABLE_PRESENT_VBLANKS)
        {
            gNdsBattlePlayablePacingPresentIntervalBucket[bucket]++;
        }
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
#if NDS_TICK_HUD
    {
        u32 draw_known = gNdsTickHudFighterTicks + gNdsTickHudStageTicks +
            gNdsTickHudBackgroundTicks + gNdsTickHudForegroundTicks;
        u32 misc_draw = (gNdsRendererProfileDrawTicks >= draw_known) ?
            (gNdsRendererProfileDrawTicks - draw_known) : 0u;
        u32 all = cpuGetTiming() - sNdsBattlePlayableTickHudLoopStartTick;
        u32 named;

        misc_draw += gNdsTickHudFlushTicks;
        /* R2-07 MISC split reader. Names all three cumulative sub-path
         * counters so --gc-sections keeps them (a debugger is not a reader),
         * and carries the running total the split accounts for -- MISC minus
         * this, differenced across two ring stops, is the part still
         * unexplained. */
        gNdsMiscSplitAccountedTicks =
            gNdsMiscWeaponDrawTicks + gNdsMiscEffectDrawTicks +
            gNdsMiscParticleDrawTicks + gNdsMiscTexUploadTicks +
            gNdsMiscTexUploadCount + gNdsMiscTexUploadBytes +
            /* R2-08 phase split, same reason: every one of these is written
             * only from a #if NDS_TICK_HUD block and read only by a debugger,
             * which is precisely the shape --gc-sections collects. Naming them
             * here is what keeps the measuring run from reading eleven zeros.
             * gNdsEffectPhaseActive is excluded deliberately -- it is a live
             * flag tested by nds_renderer.c, so it already has a reader. */
            gNdsEffectPhaseColorTicks + gNdsEffectPhaseTreeTicks +
            gNdsEffectPhaseDLTicks + gNdsEffectPhaseFindTicks +
            gNdsEffectPhaseMaterialTicks + gNdsEffectPhaseMatrixTicks +
            gNdsEffectPhaseExecTicks + gNdsEffectPhaseTexTicks +
            gNdsEffectPhaseDLCount + gNdsEffectPhaseNodeCount +
            gNdsEffectDLCommandTotal + gNdsEffectDLTermCapCount +
            gNdsEffectDLTermEndCount + gNdsEffectDLTermOtherCount +
            gNdsEffectDLTermOtherMask +
            /* G3 step 0 census, same reason again: written only from a
             * #if NDS_TICK_HUD block, read only by a debugger. */
            gNdsEffectDLCensusUnique + gNdsEffectDLCensusOverflow +
            gNdsEffectDLCensusStateVariants +
            gNdsEffectDLCensusCommandVariants +
            gNdsEffectDLCensusCommandMax +
            gNdsEffectDLCensusUniqueCommandTotal +
            gNdsEffectDLTriangleTotal + gNdsEffectDLVertexTotal +
            gNdsEffectDLCensusGeomVariants +
            gNdsEffectDLCensusTrisMaxTotal +
            gNdsEffectDLCensusVertsMaxTotal +
            /* Cycle 98 FTR pre-submission census, same reason a fourth time:
             * written only from #if NDS_TICK_HUD blocks and read only by a
             * debugger, which is exactly the shape --gc-sections collects.
             * Naming them here is what keeps the measuring run from reading
             * thirteen zeros and calling every seam already-dead. */
            gNdsFtrPreValidateReuse + gNdsFtrPreValidateBuild +
            gNdsFtrPreValidateReject + gNdsFtrPreWalkSame +
            gNdsFtrPreWalkVariant + gNdsFtrPreWalkFirst +
            gNdsFtrPreMatCalls + gNdsFtrPreMatSame +
            gNdsFtrPreMatVariant + gNdsFtrPreMatNew +
            gNdsFtrPreMatEvict + gNdsFtrPreResetTransient +
            gNdsFtrPreResetRuntime +
            /* Cycle 99 baked-plan engagement/equivalence counters, same
             * reason: written only from #if NDS_TICK_HUD blocks and read only
             * by a debugger. gNdsFtrPlanRoute/Verify are poked rather than
             * written by the guest, so they need this even more. */
            gNdsFtrPlanRoute + gNdsFtrPlanVerify + gNdsFtrPlanHit +
            gNdsFtrPlanBuild + gNdsFtrPlanVerifyRuns +
            gNdsFtrPlanVerifyMismatch;
        gNdsTickHudBuckets[nNDSTickHudBucketFighters] =
            gNdsTickHudFighterTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketStage] = gNdsTickHudStageTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketBackground] =
            gNdsTickHudBackgroundTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketAudio] = gNdsTickHudAudioTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketHud] =
            gNdsTickHudForegroundTicks + gNdsRendererProfileHudTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketSourceUpdate] =
            gNdsTickHudSourceTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketMiscDraw] = misc_draw;
        named = gNdsTickHudBuckets[nNDSTickHudBucketFighters] +
            gNdsTickHudBuckets[nNDSTickHudBucketStage] +
            gNdsTickHudBuckets[nNDSTickHudBucketBackground] +
            gNdsTickHudBuckets[nNDSTickHudBucketAudio] +
            gNdsTickHudBuckets[nNDSTickHudBucketHud] +
            gNdsTickHudBuckets[nNDSTickHudBucketSourceUpdate] +
            gNdsTickHudBuckets[nNDSTickHudBucketMiscDraw];
        gNdsTickHudBuckets[nNDSTickHudBucketAll] = all;
        gNdsTickHudBuckets[nNDSTickHudBucketOther] =
            (all >= named) ? (all - named) : 0u;
        /* Task 66. ALL and OTHR keep their exact prior meaning so every number
         * in the task ledger stays comparable; WAIT and WORK are additive.
         *
         * WAIT is the span the loop spends parked in swiWaitForVBlank, which is
         * what OTHR has really been made of -- Task 65 measured idle at 17.50%
         * of wall against an OTHR of 16.4%, and confirmed that GX backpressure
         * is not pooled here but distributed through the named buckets as
         * memory stall on the write that could not retire. WORK is the search
         * quantity: unlike ALL it is not quantized, so a saving smaller than
         * one VBlank shows up in it instead of vanishing into the wait. */
        gNdsTickHudBuckets[nNDSTickHudBucketVBlankWait] =
            gNdsTickHudVBlankWaitTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketWork] =
            (all >= gNdsTickHudVBlankWaitTicks) ?
            (all - gNdsTickHudVBlankWaitTicks) : 0u;
        /* Cycle 85 SRC split. Published AFTER `named` is summed and deliberately
         * absent from it: both spans are nested inside SRC, which `named`
         * already counts, so adding them would double-count against ALL and
         * corrupt OTHR -- the accounting remainder every excursion ranking
         * depends on. Appending here instead keeps the identity
         * WORK-H = (FTR+STG+BG+AUD+SRC+MISC) + (OTHR-WAIT) byte-identical. */
        gNdsTickHudBuckets[nNDSTickHudBucketSrcHitDetect] =
            gNdsTickHudSrcHitDetectTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketSrcAnimWarm] =
            gNdsTickHudSrcAnimWarmTicks;
        /* Cycle 86 SBAS split, published on the same terms and for the same
         * reason: nested inside SRC, so after `named` and never part of it. */
        gNdsTickHudBuckets[nNDSTickHudBucketSrcRunAll] =
            gNdsTickHudSrcRunAllTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketSrcComputer] =
            gNdsTickHudSrcComputerTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketSrcCatch] =
            gNdsTickHudSrcCatchTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketSrcParams] =
            gNdsTickHudSrcParamsTicks;
        /* Cycle 92 SGCO split, published on the same terms: nested inside GCRA,
         * so after `named` and never part of it. */
        gNdsTickHudBuckets[nNDSTickHudBucketSrcInterrupt] =
            gNdsTickHudSrcInterruptTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketSrcPhysicsDefault] =
            gNdsTickHudSrcPhysicsDefaultTicks;
        gNdsTickHudBuckets[nNDSTickHudBucketSrcPhysicsCapture] =
            gNdsTickHudSrcPhysicsCaptureTicks;
        /* Feed the HUD percentile window here, on the per-iteration path. The
         * HUD renderer only runs about twice a second, so sampling inside it
         * would build the distribution from half-second-spaced single frames
         * instead of from every presented frame. */
        ndsPlatformTickHudSample();
    }
#endif
    NDS_TASK37_PROFILE_FRAME_TICK(gNdsBattlePlayablePacingPresentedFrames);
#if NDS_R2_FLASH_PROBE
    /* R2-03 E48. Latch and reset here, on the same per-iteration path the tick
     * HUD samples from, so the branch counts are per presented frame and carry
     * the same frame numbering the capture harness stops on. */
    ndsRendererR2FlashProbeFrameEnd(gNdsBattlePlayablePacingPresentedFrames);
#endif
    /* Publish here and not one line later: the harness breaks on
     * ndsBattlePlayableFrameCompleteMarker, GDB stops at its entry, and GDB
     * cannot see the ARM9 D-cache. This is also the only point in the
     * iteration where the whole tuple is consistent -- DrawCalls,
     * PresentedFrames, PhasePresentCount, LogicFrames and the taskman update
     * count have all advanced. See NDS_PUBLISH_DEBUGGER_GROUP in
     * nds_platform.h. */
    ndsPlatformPublishBattleFrameCompleteGroups();
#if NDS_P2_FOUR_CPU_STRESS && NDS_TICK_HUD
    if ((gNdsBattlePlayablePacingPresentedFrames & 31u) == 0u)
    {
        ndsBattlePlayableTickHudSparseMarker();
    }
#endif
    ndsBattlePlayableFrameCompleteMarker();
    NDS_FREEZE_DIAGNOSTICS_HEARTBEAT();
}

#if NDS_R2_PATH
/* Runtime 2 host surface (R2-01, docs/Smash64DS_Runtime2_SwitchPlan.md).
 *
 * src/nds/r2 owns battle scene flow; Runtime 1 still owns the 60 Hz tick and
 * the draw. These are the operations the R2 loop drives, and nothing more --
 * the whole block compiles out when NDS_R2_PATH is 0, so the published arm is
 * byte-identical to a build without the family. That matters more than it
 * looks: Tasks 87-89/94/95 all regressed because editing this translation unit
 * re-addressed its neighbours, so the 0 arm must not be edited at all, only
 * added to under a flag.
 *
 * The per-iteration profile accumulators were locals in the harness loop. They
 * become file statics here because the loop that owns them now lives in
 * another translation unit; the arithmetic is unchanged. */
#if NDS_RENDERER_PROFILE_LEVEL >= 1
static u32 sNdsR2ProfileInputTicks;
static u32 sNdsR2ProfileUpdateTicks;
static u32 sNdsR2ProfileSourceUpdateByIndex[2];
static u32 sNdsR2ProfileSourceUpdateTicks;
static u32 sNdsR2ProfileAudioUpdateTicks;
#endif

void ndsR2HostBattlePrepare(void)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    ndsStageCollisionLoopPrepareRuntime();
#if NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS
    ndsAudioAssetLoadFenced();
#endif
    ndsFighterMarioFoxNaturalMotionPrepare();
#endif
    ndsBattlePlayablePacingStart(0u);
}

void ndsR2HostBattleIterationBegin(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsR2ProfileInputTicks = 0u;
    sNdsR2ProfileUpdateTicks = 0u;
    sNdsR2ProfileSourceUpdateByIndex[0] = 0u;
    sNdsR2ProfileSourceUpdateByIndex[1] = 0u;
    sNdsR2ProfileSourceUpdateTicks = 0u;
    sNdsR2ProfileAudioUpdateTicks = 0u;
    sNdsBattlePlayableProfileLoopStartTick = cpuGetTiming();
#endif
#if NDS_TICK_HUD
    sNdsBattlePlayableTickHudLoopStartTick = cpuGetTiming();
    gNdsTickHudFighterTicks = 0u;
    gNdsTickHudStageTicks = 0u;
    gNdsTickHudBackgroundTicks = 0u;
    gNdsTickHudForegroundTicks = 0u;
    gNdsTickHudAudioTicks = 0u;
    gNdsTickHudSourceTicks = 0u;
    gNdsTickHudSrcHitDetectTicks = 0u;
    gNdsTickHudSrcAnimWarmTicks = 0u;
    gNdsTickHudSrcRunAllTicks = 0u;
    gNdsTickHudSrcComputerTicks = 0u;
    gNdsTickHudSrcCatchTicks = 0u;
    gNdsTickHudSrcParamsTicks = 0u;
    gNdsTickHudSrcInterruptTicks = 0u;
    gNdsTickHudSrcPhysicsDefaultTicks = 0u;
    gNdsTickHudSrcPhysicsCaptureTicks = 0u;
    gNdsTickHudFlushTicks = 0u;
    gNdsTickHudVBlankWaitTicks = 0u;
#endif
    /* The lifecycle verifier requests this only after its synchronized
     * MATCH_START stop, so debugger time is never reported as slowdown. */
    if (gNdsBattlePlayablePacingRestartRequested != 0u)
    {
        ndsBattlePlayablePacingStart(0u);
        gNdsBattlePlayablePacingRestartRequested = 0u;
    }
}

u32 ndsR2HostBattleUpdateOnce(u32 update_index)
{
#if NDS_R2_POSITION_PROBE
    /* Mirrors the Runtime 1 loop, which publishes the capture index in the
     * same position -- immediately before its own `battle_status_before` read.
     * Without this the R2 path leaves the index at 0, so every hurtbox capture
     * below reads as "tick 0" and the probe silently reports one substep. */
    gNdsPositionProbeUpdateInPresent = update_index;
#endif
    u32 battle_status_before = (gSCManagerBattleState != NULL) ?
        (u32)gSCManagerBattleState->game_status : 0xffffffffu;
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 input_start = cpuGetTiming();
#endif

    /* Slice 1 phase 7's gate, from a read this function already performs. One
     * logic update of lag at the countdown edge, which can only ADD to the
     * pre-GO side; every acquisition inside a GO update is counted. */
    gNdsK0BattleInGo =
        (battle_status_before == (u32)nSCBattleGameStatusGo) ? 1u : 0u;
    NDS_FREEZE_DIAGNOSTICS_MARK(NDS_FREEZE_BREADCRUMB_UPDATE_START);
    (void)ndsPlatformReadInput();
    if (NDS_DEV_LIVE_INPUT_PREVIEW != 0)
    {
        syControllerReadDeviceData();
        syControllerUpdateGlobalData();
    }
    ndsBattlePlayableAdvanceRealtimeLogicClock();
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsR2ProfileInputTicks += cpuGetTiming() - input_start;
#endif
    /* P2-2p6: the fighter pose engine evaluates body joints on the LAST source
     * tick of a presented frame -- the pose the present will draw -- and holds
     * them on the others. The Runtime 1 loop publishes the same word. */
    gNdsFtPoseEvalTick =
        ((update_index + 1u) >= ndsR2HostBattleUpdatesPerPresent()) ? 1u : 0u;
    ndsRunMarioFoxProofUpdate(&gNdsFighterGCRunAllLoopTaskmanUpdateCount);
#if NDS_R2_POSITION_PROBE
    if ((gSCManagerBattleState != NULL) &&
        (gSCManagerBattleState->players[0].fighter_gobj != NULL))
    {
        ndsPositionProbeCaptureMarioHurtboxes(
            gSCManagerBattleState->players[0].fighter_gobj);
    }
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    sNdsR2ProfileUpdateTicks += gNdsRendererProfileUpdateTicks;
    if (update_index < 2u)
    {
        sNdsR2ProfileSourceUpdateByIndex[update_index] =
            gNdsRendererProfileSourceUpdateTicks;
    }
    sNdsR2ProfileSourceUpdateTicks += gNdsRendererProfileSourceUpdateTicks;
    sNdsR2ProfileAudioUpdateTicks += gNdsRendererProfileAudioUpdateTicks;
#else
    (void)update_index;
#endif
    if ((battle_status_before == nSCBattleGameStatusWait) &&
        (gSCManagerBattleState != NULL) &&
        (gSCManagerBattleState->game_status == nSCBattleGameStatusGo))
    {
        /* Keep all prepare-once countdown atlases resident. */
        ndsRendererHardwareArmBattleStaticTextures();
    }
    /* BattleShip syTaskmanRunTask checks LoadScene immediately after
     * task_update and never draws the terminal update. */
    return (sSYTaskmanStatus == nSYTaskmanStatusLoadScene) ? 1u : 0u;
}

void ndsR2HostBattlePresent(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    gNdsRendererProfileInputTicks = sNdsR2ProfileInputTicks;
    gNdsRendererProfileUpdateTicks = sNdsR2ProfileUpdateTicks;
    gNdsRendererProfileSourceUpdate1Ticks = sNdsR2ProfileSourceUpdateByIndex[0];
    gNdsRendererProfileSourceUpdate2Ticks = sNdsR2ProfileSourceUpdateByIndex[1];
    gNdsRendererProfileSourceUpdateTicks = sNdsR2ProfileSourceUpdateTicks;
    gNdsRendererProfileAudioUpdateTicks = sNdsR2ProfileAudioUpdateTicks;
#endif
    ndsBattlePlayablePresentRealtimeFrame();
    /* Count only updates committed to a presented frame; the source-faithful
     * LoadScene terminal update remains undrawn. */
    gNdsBattlePlayablePacingLogicFrames +=
        NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT;
    ndsBattlePlayableFinalizePresentedIteration();
}

void ndsR2HostBattleFinish(void)
{
    ndsBattlePlayableRecordLifecycleTaskmanExit();
    ndsBattlePlayablePacingFinish();
}

u32 ndsR2HostBattleNaturalMotionPassed(void)
{
    return (gNdsFighterNaturalMotionResult ==
            NDS_FIGHTER_NATURAL_MOTION_PASS) ? 1u : 0u;
}

u32 ndsR2HostBattleUpdatesPerPresent(void)
{
    return NDS_BATTLE_PLAYABLE_REALTIME_UPDATES_PER_PRESENT;
}

u32 ndsR2HostBattleUpdateMax(void)
{
    return (NDS_DEV_LIVE_INPUT_PREVIEW != 0) ?
        NDS_FIGHTER_BATTLE_PLAYABLE_LIVE_UPDATE_MAX :
        NDS_FIGHTER_BATTLE_PLAYABLE_REALTIME_SMOKE_UPDATE_MAX;
}
#endif /* NDS_R2_PATH */
