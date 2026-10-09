#include "nds_scene_harness_config.h"

#include <nds/nds_freeze_diagnostics.h>
#include <nds/nds_menu_shell.h>
#include <nds/nds_ifcommon_oam.h>
#include <nds/nds_source2d.h>
#include <gr/ground.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_task37_profile.h>
#if NDS_R2_COLLISION_L7_ORACLE
#include <nds/nds_r2_collision_oracle.h>
#endif

#if NDS_R2_PATH
#include <nds/nds_r2_battle.h>
/* R2-01. The R2 loop is specialized for the Boundary configuration: it folds
 * `is_battle_playable` and `use_realtime_presentation` to 1. Both are only
 * constant under this harness, so any other harness would silently get a loop
 * that does not match it. Fail the build closed rather than ship that. */
#if NDS_DEV_SCENE_HARNESS != NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE
#error "NDS_R2_PATH=1 requires the battle_playable harness (mode 163)"
#endif
#if NDS_HARNESS_FAST_LOGIC
#error "NDS_R2_PATH=1 is the realtime path; NDS_HARNESS_FAST_LOGIC must be 0"
#endif
#endif

extern u32 sySchedulerGetTicCount(void);
extern void sySchedulerSetTicCount(u32 tics);

#if NDS_R2_POSITION_PROBE
/* Probe-only: which of the two unchanged 60 Hz source ticks inside the current
 * 30 Hz present is executing. Attachment effects sampled in tick 0 but drawn
 * after tick 1 can otherwise look like a transform bug even when both source
 * computations are individually exact. */
__attribute__((used)) volatile u32 gNdsPositionProbeUpdateInPresent;
extern void ndsPositionProbeCaptureMarioHurtboxes(GObj *fighter_gobj);
#endif

#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
extern void ndsMNVSResultsRecordFrame(void);
extern void ndsSObjPreviewBeginFrame(void);
extern void ndsSObjPreviewEndFrame(void);
#endif

/* A shipping build (NDS_DIAG_COUNTERS=0) keeps only the non-zero resets
 * below: ndsResetStartupDiagnostics runs once, from the startup scene, and
 * the zero resets only clear what .bss already holds there. The development
 * targets keep every store, in order. */
#if NDS_DIAG_COUNTERS
#define NDS_DIAG_RESET0(name) ((name) = 0)
#define NDS_DIAG_WORD0(name) NDS_DIAG_WORD(name, 0u),
#else
#define NDS_DIAG_RESET0(name) ((void)0)
#define NDS_DIAG_WORD0(name)
#endif

/* Preserve store order while sharing the repeated cold counter-reset code.
 * The source's 25 KiB effect reserve competes with this ~34 KiB initializer.
 * Aligned integer-word addresses carry tags: 0 -> zero, 1 -> all-ones, 2 -> one.
 * may_alias covers both the platform uint32_t and BattleShip u32 spellings. */
typedef u32 NDSDiagnosticWord __attribute__((may_alias));
#define NDS_DIAG_WORD(name, tag) ((uintptr_t)&(name) + (tag) + 0u * sizeof(char[ \
    (sizeof(name) == 4 && __builtin_classify_type(name) == 1 && \
     __alignof__(name) >= 4) ? 1 : -1]))
static void __attribute__((noinline, noclone)) ndsResetDiagnosticWords(
    const uintptr_t *words, u32 count)
{
    u32 i;
    for (i = 0u; i < count; i++)
    {
        uintptr_t word = words[i];
        u32 value = (word & 1u) ? 0xffffffffu : (u32)((word & 2u) >> 1);
        *(volatile NDSDiagnosticWord *)(word & ~(uintptr_t)3u) = value;
    }
}

void ndsResetStartupDiagnostics(void)
{
    NDS_DIAG_RESET0(gNdsSceneBoundaryResult);
    NDS_DIAG_RESET0(gNdsSceneBoundaryKind);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStartupTaskmanResult)
            NDS_DIAG_WORD0(gNdsStartupTaskmanSceneKind)
            NDS_DIAG_WORD0(gNdsStartupTaskmanDL0Size)
            NDS_DIAG_WORD0(gNdsStartupTaskmanDL1Size)
            NDS_DIAG_WORD0(gNdsStartupTaskmanControllerSet)
            NDS_DIAG_WORD0(gNdsStartupFuncStartResult)
            NDS_DIAG_WORD0(gNdsStartupSkipAllowWait)
            NDS_DIAG_WORD0(gNdsStartupProceedOpening)
            NDS_DIAG_WORD0(gNdsStartupGObjCreateCount)
            NDS_DIAG_WORD0(gNdsStartupCameraCreateCount)
            NDS_DIAG_WORD0(gNdsStartupRelocInitCount)
            NDS_DIAG_WORD0(gNdsStartupSpriteCreateCount)
            NDS_DIAG_WORD0(gNdsStartupFadeCreateCount)
            NDS_DIAG_WORD0(gNdsStartupWallpaperParentValid)
            NDS_DIAG_WORD0(gNdsStartupLogoPosX)
            NDS_DIAG_WORD0(gNdsStartupLogoPosY)
            NDS_DIAG_WORD0(gNdsStartupLogoFastcopyCleared)
            NDS_DIAG_WORD0(gNdsStartupLogoRelocResult)
            NDS_DIAG_WORD0(gNdsStartupLogoRelocSize)
            NDS_DIAG_WORD0(gNdsStartupLogoRelocWordSwapCount)
            NDS_DIAG_WORD0(gNdsStartupLogoRelocPointerFixupCount)
            NDS_DIAG_WORD0(gNdsStartupLogoDrawResult)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsStartupLogoDrawBlocker = NDS_STARTUP_LOGO_BLOCKER_NONE;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStartupLogoDrawCallbackCount)
            NDS_DIAG_WORD0(gNdsStartupLogoDrawUpdateCount)
            NDS_DIAG_WORD0(gNdsStartupLogoDrawWidth)
            NDS_DIAG_WORD0(gNdsStartupLogoDrawHeight)
            NDS_DIAG_WORD(gNdsStartupLogoDrawFormat, 1u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawSize, 1u),
            NDS_DIAG_WORD0(gNdsStartupLogoDrawBitmaps)
            NDS_DIAG_WORD0(gNdsStartupLogoDrawPixels)
            NDS_DIAG_WORD(gNdsStartupLogoDrawGObjID, 1u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawGObjObjKind, 1u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawSObjAttr, 1u),
            NDS_DIAG_WORD0(gNdsStartupLogoDrawTexshuf)
            NDS_DIAG_WORD0(gNdsStartupLogoDrawTexshufSamples)
            NDS_DIAG_WORD0(gNdsStartupLogoDrawVisibleSObjCount)
            NDS_DIAG_WORD0(gNdsStartupActorFuncSet)
            NDS_DIAG_WORD(gNdsStartupWallpaperProcessKind, 1u),
            NDS_DIAG_WORD(gNdsStartupWallpaperProcessPriority, 1u),
            NDS_DIAG_WORD0(gNdsStartupWallpaperDisplaySet)
            NDS_DIAG_WORD0(gNdsStartupWallpaperCameraMaskLow)
            NDS_DIAG_WORD0(gNdsStartupDefaultCameraColor)
            NDS_DIAG_WORD0(gNdsTaskmanBridgeResult)
            NDS_DIAG_WORD0(gNdsTaskmanContexts)
            NDS_DIAG_WORD0(gNdsTaskmanTaskGfxNum)
            NDS_DIAG_WORD0(gNdsTaskmanGraphicsHeapSize)
            NDS_DIAG_WORD0(gNdsTaskmanRdpKind)
            NDS_DIAG_WORD0(gNdsTaskmanRdpBufferSize)
            NDS_DIAG_WORD0(gNdsTaskmanMallocCount)
            NDS_DIAG_WORD0(gNdsStartupTaskmanMallocCount)
            NDS_DIAG_WORD0(gNdsTaskmanGeneralHeapUsed)
            NDS_DIAG_WORD0(gNdsTaskmanDLContextsValid)
            NDS_DIAG_WORD0(gNdsTaskmanControllerAutoRead)
            NDS_DIAG_WORD0(gNdsTaskmanSceneUpdateSet)
            NDS_DIAG_WORD0(gNdsTaskmanSceneDrawSet)
            NDS_DIAG_WORD0(gNdsTaskmanLightsSet)
            NDS_DIAG_WORD0(gNdsTaskmanLoopReached)
            NDS_DIAG_WORD0(gNdsTaskmanBoundedUpdateCount)
            NDS_DIAG_WORD0(gNdsTaskmanPostUpdateSkip)
            NDS_DIAG_WORD0(gNdsTaskmanGObjThreadSleeps)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsOsGObjThreadProvisionCount);
    NDS_DIAG_RESET0(gNdsOsGObjThreadProvisionFailCount);
    NDS_DIAG_RESET0(gNdsOsThreadHeapCreateCount);
    NDS_DIAG_RESET0(gNdsOsStartThreadNoEntryCount);
    NDS_DIAG_RESET0(gNdsOsStartThreadCreateFailCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsTaskmanPostUpdateLogoPosX)
            NDS_DIAG_WORD0(gNdsTaskmanPostUpdateLogoPosY)
            NDS_DIAG_WORD0(gNdsTaskmanPostUpdateOpening)
            NDS_DIAG_WORD0(gNdsTaskmanPostUpdateSceneKind)
            NDS_DIAG_WORD0(gNdsTaskmanPostUpdateScenePrev)
            NDS_DIAG_WORD0(gNdsTaskmanPostUpdateStatus)
            NDS_DIAG_WORD0(gNdsTaskmanPostUpdateGObjCount)
            NDS_DIAG_WORD0(gNdsTaskmanPostUpdateFadeCount)
            NDS_DIAG_WORD0(gNdsTaskmanCleanupResult)
            NDS_DIAG_WORD0(gNdsTaskmanCleanupQueuesEmpty)
            NDS_DIAG_WORD0(gNdsTaskmanCleanupMode)
            NDS_DIAG_WORD0(gNdsTaskmanReturnCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsMemoryLedgerResult);
    NDS_DIAG_RESET0(gNdsMemoryLedgerScene);
    NDS_DIAG_RESET0(gNdsMemoryLedgerGeneration);
    NDS_DIAG_RESET0(gNdsMemoryLedgerArenaCapacity);
    NDS_DIAG_RESET0(gNdsMemoryLedgerArenaUsed);
    NDS_DIAG_RESET0(gNdsMemoryLedgerArenaHighWater);
    NDS_DIAG_RESET0(gNdsMemoryLedgerArenaHeadroom);
    NDS_DIAG_RESET0(gNdsMemoryLedgerDLBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerGraphicsBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRdpBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerFigatreeHeapSize);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRelocFiles);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRelocBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRelocStageBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRelocFighterBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRelocInterfaceBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRelocMenuBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRelocOpeningBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRelocOtherBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRelocStaleFiles);
    NDS_DIAG_RESET0(gNdsMemoryLedgerRelocStaleBytes);
    NDS_DIAG_RESET0(gNdsMemoryLedgerEvictedFiles);
    NDS_DIAG_RESET0(gNdsMemoryLedgerEvictedBytes);
    ndsAudioAssetDiagnosticsReset();
    ndsAudioBgmDiagnosticsReset();
    NDS_DIAG_RESET0(gNdsOpeningRoomDispatchCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomStartResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomFuncStartResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomUpdateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomTickCount);
    NDS_DIAG_RESET0(gNdsOpeningMovieRoomHandoffResult);
    NDS_DIAG_RESET0(gNdsOpeningMovieRoomHandoffTick);
    NDS_DIAG_RESET0(gNdsOpeningMovieRoomHandoffScene);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDispatchCount);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsStartResult);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsFuncStartResult);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsUpdateResult);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsTickCount);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsRelocResult);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsSpriteNormalizeCount);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsSpriteNormalizeFailCount);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDrawResult);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDrawBlocker);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDrawCallbackCount);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDrawVisibleSObjCount);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDrawWidth);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDrawHeight);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDrawFormat);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDrawSize);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDrawBitmaps);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsDrawPixels);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsNextSceneResult);
    NDS_DIAG_RESET0(gNdsOpeningPortraitsNextSceneKind);
    NDS_DIAG_RESET0(gNdsOpeningMarioDispatchCount);
    NDS_DIAG_RESET0(gNdsOpeningMarioStartResult);
    NDS_DIAG_RESET0(gNdsOpeningMarioFuncStartResult);
    NDS_DIAG_RESET0(gNdsOpeningMarioUpdateResult);
    NDS_DIAG_RESET0(gNdsOpeningMarioTickCount);
    NDS_DIAG_RESET0(gNdsOpeningMarioRelocResult);
    NDS_DIAG_RESET0(gNdsOpeningMarioSpriteNormalizeCount);
    NDS_DIAG_RESET0(gNdsOpeningMarioSpriteNormalizeFailCount);
    NDS_DIAG_RESET0(gNdsOpeningMarioDrawResult);
    NDS_DIAG_RESET0(gNdsOpeningMarioDrawBlocker);
    NDS_DIAG_RESET0(gNdsOpeningMarioDrawCallbackCount);
    NDS_DIAG_RESET0(gNdsOpeningMarioDrawVisibleSObjCount);
    NDS_DIAG_RESET0(gNdsOpeningMarioDrawWidth);
    NDS_DIAG_RESET0(gNdsOpeningMarioDrawHeight);
    NDS_DIAG_RESET0(gNdsOpeningMarioDrawFormat);
    NDS_DIAG_RESET0(gNdsOpeningMarioDrawSize);
    NDS_DIAG_RESET0(gNdsOpeningMarioDrawBitmaps);
    NDS_DIAG_RESET0(gNdsOpeningMarioDrawPixels);
    NDS_DIAG_RESET0(gNdsOpeningMarioFighterDeferredResult);
    NDS_DIAG_RESET0(gNdsOpeningMarioFighterDeferredTick);
    NDS_DIAG_RESET0(gNdsOpeningMarioNextSceneResult);
    NDS_DIAG_RESET0(gNdsOpeningMarioNextSceneKind);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDispatchMask);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneFuncStartMask);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneUpdateMask);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneFighterDeferMask);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneNextMask);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawMask);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDispatchCount);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneLastKind);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneLastTick);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneLastNextKind);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawResult);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawBlocker);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawCallbackCount);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawVisibleSObjCount);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawWidth);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawHeight);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawFormat);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawSize);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawBitmaps);
    NDS_DIAG_RESET0(gNdsOpeningNameSceneDrawPixels);
    NDS_DIAG_RESET0(gNdsOpeningMovieBridgeResult);
    NDS_DIAG_RESET0(gNdsOpeningMovieBridgeMask);
    NDS_DIAG_RESET0(gNdsOpeningMovieBridgeCount);
    NDS_DIAG_RESET0(gNdsOpeningMovieBridgeLastKind);
    NDS_DIAG_RESET0(gNdsOpeningMovieBridgeLastNextKind);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewResult);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewMask);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewCount);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewFrameCount);
    NDS_DIAG_RESET0(gNdsOpeningMoviePresentFrameCount);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewPixels);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewSpriteNormalizeCount);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewSpriteNormalizeFailCount);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewLastKind);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewLastWidth);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewLastHeight);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewLastFormat);
    NDS_DIAG_RESET0(gNdsOpeningMovieActionPreviewLastSize);
    NDS_DIAG_RESET0(gNdsOpeningMovieTitleResult);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsTitleRelocResult)
            NDS_DIAG_WORD0(gNdsTitlePreviewResult)
            NDS_DIAG_WORD0(gNdsTitleDrawResult)
            NDS_DIAG_WORD0(gNdsTitleSpriteNormalizeCount)
            NDS_DIAG_WORD0(gNdsTitleSpriteNormalizeFailCount)
            NDS_DIAG_WORD0(gNdsTitleFireSpriteNormalizeCount)
            NDS_DIAG_WORD0(gNdsTitleFireSpriteNormalizeFailCount)
            NDS_DIAG_WORD0(gNdsTitleDrawVisibleSObjCount)
            NDS_DIAG_WORD0(gNdsTitleDrawRenderableSObjCount)
            NDS_DIAG_WORD0(gNdsTitleDrawSObjCount)
            NDS_DIAG_WORD0(gNdsTitleDrawPixels)
            NDS_DIAG_WORD0(gNdsTitleDrawLastWidth)
            NDS_DIAG_WORD0(gNdsTitleDrawLastHeight)
            NDS_DIAG_WORD0(gNdsTitleDrawLastFormat)
            NDS_DIAG_WORD0(gNdsTitleDrawLastSize)
            NDS_DIAG_WORD0(gNdsTitleOriginalStartResult)
            NDS_DIAG_WORD0(gNdsTitleOriginalFuncStartResult)
            NDS_DIAG_WORD0(gNdsTitleOriginalSetupMask)
            NDS_DIAG_WORD0(gNdsTitleOriginalLoadedFileCount)
            NDS_DIAG_WORD0(gNdsTitleOriginalGObjCount)
            NDS_DIAG_WORD0(gNdsTitleOriginalCameraCount)
            NDS_DIAG_WORD0(gNdsTitleOriginalMainGObjID)
            NDS_DIAG_WORD0(gNdsTitleOriginalTransitionGObjID)
            NDS_DIAG_WORD0(gNdsTitleOriginalDeferredMask)
            NDS_DIAG_WORD0(gNdsTitleOriginalLogoFireResult)
            NDS_DIAG_WORD0(gNdsTitleOriginalLogoFireMask)
            NDS_DIAG_WORD0(gNdsTitleOriginalLogoFireGObjDelta)
            NDS_DIAG_WORD0(gNdsTitleOriginalLogoFireLinkID)
            NDS_DIAG_WORD0(gNdsTitleOriginalLogoFireDLLinkID)
            NDS_DIAG_WORD0(gNdsTitleOriginalLogoFireCameraMaskLo)
            NDS_DIAG_WORD0(gNdsTitleOriginalLogoFireParticleBank)
            NDS_DIAG_WORD0(gNdsTitleOriginalFireResult)
            NDS_DIAG_WORD0(gNdsTitleOriginalFireMask)
            NDS_DIAG_WORD0(gNdsTitleOriginalFireGObjDelta)
            NDS_DIAG_WORD0(gNdsTitleOriginalFireSObjDelta)
            NDS_DIAG_WORD0(gNdsTitleOriginalFireGObjFlags)
            NDS_DIAG_WORD0(gNdsTitleOriginalFireSObjCount)
            NDS_DIAG_WORD0(gNdsTitleOriginalFireFrames)
            NDS_DIAG_WORD0(gNdsTitleOriginalFireAlpha)
            NDS_DIAG_WORD0(gNdsTitleOriginalUpdateResult)
            NDS_DIAG_WORD0(gNdsTitleOriginalUpdateCount)
            NDS_DIAG_WORD0(gNdsTitleOriginalLayout)
            NDS_DIAG_WORD0(gNdsTitleOriginalTransitionTics)
            NDS_DIAG_WORD0(gNdsTitleOriginalStartActorProcess)
            NDS_DIAG_WORD0(gNdsTitleOriginalProceedScene)
            NDS_DIAG_WORD0(gNdsTitleOriginalProceedWait)
            NDS_DIAG_WORD0(gNdsVSModeOriginalStartResult)
            NDS_DIAG_WORD0(gNdsVSModeOriginalFuncStartResult)
            NDS_DIAG_WORD0(gNdsVSModeOriginalRelocResult)
            NDS_DIAG_WORD0(gNdsVSModeOriginalSetupResult)
            NDS_DIAG_WORD0(gNdsVSModeOriginalSetupMask)
            NDS_DIAG_WORD0(gNdsVSModeOriginalLoadedFileCount)
            NDS_DIAG_WORD0(gNdsVSModeOriginalGObjCount)
            NDS_DIAG_WORD0(gNdsVSModeOriginalCameraCount)
            NDS_DIAG_WORD0(gNdsVSModeOriginalSObjCount)
            NDS_DIAG_WORD0(gNdsVSModeOriginalMainGObjID)
            NDS_DIAG_WORD0(gNdsVSModeOriginalCursorIndex)
            NDS_DIAG_WORD0(gNdsVSModeOriginalRule)
            NDS_DIAG_WORD0(gNdsVSModeOriginalTime)
            NDS_DIAG_WORD0(gNdsVSModeOriginalStock)
            NDS_DIAG_WORD0(gNdsVSModeOriginalButtonMask)
            NDS_DIAG_WORD0(gNdsVSModeOriginalDeferredMask)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionResult)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionMask)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionUpdateCount)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionInputMask)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionScenePrevBefore)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionSceneCurrBefore)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionScenePrevAfterTap)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionSceneCurrAfterTap)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionScenePrevFinal)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionSceneCurrFinal)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionExitInterrupt)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionTaskmanStatus)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionSavedRule)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionSavedTime)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionSavedStock)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionButtonMaskAfter)
            NDS_DIAG_WORD0(gNdsVSModeStartTransitionCleanupCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalStartResult);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalFuncStartResult);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalRelocResult);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalSetupResult);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalSetupMask);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalLoadedFileCount);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalGObjCount);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalCameraCount);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalSObjCount);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalMainGObjID);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalControllerOrderMask);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalSlotKindMask);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalSlotSelectedMask);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalCursorCount);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalPuckCount);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalGateCount);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalPortraitCount);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalFigatreeHeapCount);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalTime);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalStock);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalGameRule);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalIsTeam);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalIsStageSelect);
    NDS_DIAG_RESET0(gNdsPlayersVSOriginalDeferredMask);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionResult);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionMask);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionUpdateCount);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionInputMask);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionScenePrevBefore);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionSceneCurrBefore);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionScenePrevFinal);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionSceneCurrFinal);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionPlayerCount);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionCpuCount);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionP0FKind);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionP1FKind);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionStageSelect);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionTaskmanStatus);
    NDS_DIAG_RESET0(gNdsPlayersVSReadyTransitionCleanupCount);
    NDS_DIAG_RESET0(gNdsMapsOriginalStartResult);
    NDS_DIAG_RESET0(gNdsMapsOriginalFuncStartResult);
    NDS_DIAG_RESET0(gNdsMapsOriginalRelocResult);
    NDS_DIAG_RESET0(gNdsMapsOriginalSetupResult);
    NDS_DIAG_RESET0(gNdsMapsOriginalSetupMask);
    NDS_DIAG_RESET0(gNdsMapsOriginalLoadedFileCount);
    NDS_DIAG_RESET0(gNdsMapsOriginalGObjCount);
    NDS_DIAG_RESET0(gNdsMapsOriginalCameraCount);
    NDS_DIAG_RESET0(gNdsMapsOriginalSObjCount);
    NDS_DIAG_RESET0(gNdsMapsOriginalMainGObjID);
    NDS_DIAG_RESET0(gNdsMapsOriginalCursorSlot);
    NDS_DIAG_RESET0(gNdsMapsOriginalGroundKind);
    NDS_DIAG_RESET0(gNdsMapsOriginalIsTrainingMode);
    NDS_DIAG_RESET0(gNdsMapsOriginalPreviewDeferred);
    NDS_DIAG_RESET0(gNdsMapsOriginalPreviewResult);
    NDS_DIAG_RESET0(gNdsMapsOriginalPreviewMask);
    NDS_DIAG_RESET0(gNdsMapsOriginalPreviewGObjCount);
    NDS_DIAG_RESET0(gNdsMapsOriginalPreviewLayerGObjMask);
    NDS_DIAG_RESET0(gNdsMapsOriginalPreviewWallpaperMade);
    NDS_DIAG_RESET0(gNdsMapsOriginalPreviewModelMade);
    NDS_DIAG_RESET0(gNdsMapsOriginalPreviewDObjCount);
    NDS_DIAG_RESET0(gNdsMapsOriginalPreviewMObjCount);
    NDS_DIAG_RESET0(gNdsMapsOriginalDeferredMask);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionResult);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionMask);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionUpdateCount);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionInputMask);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionScenePrevBefore);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionSceneCurrBefore);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionScenePrevFinal);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionSceneCurrFinal);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionSelectedSlot);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionSelectedGKind);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionTaskmanStatus);
    NDS_DIAG_RESET0(gNdsMapsSelectTransitionCleanupCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalStartResult);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalFuncStartResult);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalRelocResult);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalSetupResult);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalSetupMask);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalLoadedFileCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalGObjCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalCameraCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalMainGObjID);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalFighterGObjCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalActivePlayerMask);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalFighterKinds);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalPlayerCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalActivePlayerCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalFighterCreateCount);
    gNdsSCVSBattleOriginalP0FKind = 0xffffffffu;
    gNdsSCVSBattleOriginalP1FKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalP0LR);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalP1LR);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalCpuCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalGameRule);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalTime);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalStock);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalIsTeam);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalGKind);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalScenePrev);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalSceneCurr);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalUpdateResult);
    NDS_DIAG_RESET0(gNdsSCVSBattleOriginalUpdateCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleResult);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleArenaAdapterCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleTaskmanExitCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleTaskmanStatus);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleTimeLimit);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleTimeRemain);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleTimePassed);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleGameStatus);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleScenePrev);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleSceneCurr);
    NDS_DIAG_RESET0(gNdsSCVSBattleLifecycleIsSuddenDeath);
    NDS_DIAG_RESET0(gNdsSCVSBattleCompatMask);
    NDS_DIAG_RESET0(gNdsSCVSBattleCompatCameraMask);
    NDS_DIAG_RESET0(gNdsSCVSBattleCompatInterfaceMask);
    NDS_DIAG_RESET0(gNdsSCVSBattleCompatManagerMask);
    NDS_DIAG_RESET0(gNdsSCVSBattleCompatAudioMask);
    NDS_DIAG_RESET0(gNdsSCVSBattleCompatSpawnMask);
    NDS_DIAG_RESET0(gNdsSCVSBattleLastAudioVolume);
    NDS_DIAG_RESET0(gNdsSCVSBattleLastFGM);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStagePupupuRelocResult)
            NDS_DIAG_WORD0(gNdsStagePupupuRelocAssetMask)
            NDS_DIAG_WORD0(gNdsStagePupupuRelocDependencyMask)
            NDS_DIAG_WORD0(gNdsStagePupupuExternalFixupCount)
            NDS_DIAG_WORD0(gNdsStagePupupuExternalFixupFailCount)
            NDS_DIAG_WORD0(gNdsStagePupupuInternalFixupCount)
            NDS_DIAG_WORD0(gNdsStagePupupuMapHeaderOffset)
            NDS_DIAG_WORD0(gNdsStagePupupuGroundDataPtrReady)
            NDS_DIAG_WORD0(gNdsStagePupupuWallpaperPtrReady)
            NDS_DIAG_WORD0(gNdsStagePupupuGeometryPtrReady)
            NDS_DIAG_WORD0(gNdsStagePupupuMapNodesPtrReady)
            NDS_DIAG_WORD0(gNdsStagePupupuLightAngleXBits)
            NDS_DIAG_WORD0(gNdsStagePupupuLightAngleYBits)
            NDS_DIAG_WORD0(gNdsStagePupupuBGM)
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjSourceCount)
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjDecodedMask)
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjDuplicateMask)
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjUnalignedReadCount)
            NDS_DIAG_WORD(gNdsStagePupupuMapObjSourceIndices[0], 1u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjSourceIndices[1], 1u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjSourceIndices[2], 1u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjSourceIndices[3], 1u),
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjXs[0])
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjXs[1])
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjXs[2])
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjXs[3])
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjYs[0])
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjYs[1])
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjYs[2])
            NDS_DIAG_WORD0(gNdsStagePupupuMapObjYs[3])
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxRelocResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxRelocAssetMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxRelocDependencyMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxLoadedFileCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxExternalFixupCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxExternalFixupFailCount);
    NDS_DIAG_RESET0(gNdsFighterManagerResult);
    NDS_DIAG_RESET0(gNdsFighterManagerMask);
    NDS_DIAG_RESET0(gNdsFighterManagerExternMask);
    NDS_DIAG_RESET0(gNdsFighterManagerStatusBufferMask);
    NDS_DIAG_RESET0(gNdsFighterManagerFighterMask);
    NDS_DIAG_RESET0(gNdsFighterManagerDataMask);
    NDS_DIAG_RESET0(gNdsFighterManagerWaitMask);
    NDS_DIAG_RESET0(gNdsFighterManagerEntryMask);
    NDS_DIAG_RESET0(gNdsFighterManagerStatusBufferHitCount);
    NDS_DIAG_RESET0(gNdsFighterManagerFighterCount);
    NDS_DIAG_RESET0(gNdsFighterManagerFigatreeHeapSize);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionResult);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionSafeResult);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionMask);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionPrepared);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionBaseVSBattleUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionRunAllCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionControllerReadCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionManagerMask);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionGObjCountBefore);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionGObjCountAfter);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionGObjDelta);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0StatusStart);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1StatusStart);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0StatusFinal);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1StatusFinal);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0MotionFinal);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1MotionFinal);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0GAFinal);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1GAFinal);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0WaitFrameCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1WaitFrameCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0AnimAdvanceCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1AnimAdvanceCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0ValidJointCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1ValidJointCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0AnimStartBits);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1AnimStartBits);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0AnimFinalBits);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1AnimFinalBits);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionWalkInputFrame);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0WalkFrameCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1WalkFrameCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0WalkStatus);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1WalkStatus);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP0WalkMotion);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionP1WalkMotion);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionFigatreeAttachCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionFigatreeNullCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionFigatreeTableInvalidCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionFigatreeAnimInvalidCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalMotionUnsafeCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatPhase);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatPhaseFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatStallCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatApproachDXMilli);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatAttackerSlot);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatVictimSlot);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatP0DashFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatP1DashFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatP0RunFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatP1RunFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatP0RunBrakeFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatP1RunBrakeFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatP0TurnFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatP1TurnFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatP0HitlagFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatP1HitlagFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatAttackStatusFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatAttackMotionFinal);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatHitboxActiveFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatAttackRetryCount);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatVictimDamageStatus);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatVictimDamageFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatVictimStartPercent);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatVictimFinalPercent);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatVictimKnockbackMilli);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatVictimRecoverWaitFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatGuardOnFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatGuardFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatGuardOffFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatRollFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatRollStatus);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatAppealFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalCombatAppealStatus);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofResult);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofMask);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofActorSlot);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofActorKind);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofBPressFrames);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofSpecialStatusFrames);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofSpecialMotion);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofAccessoryFrames);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofFlag0Frames);
    NDS_DIAG_RESET0(gNdsFighterDamageFireCallCount);
    NDS_DIAG_RESET0(gNdsFighterEffectKindMask0);
    NDS_DIAG_RESET0(gNdsFighterEffectKindMask1);
    NDS_DIAG_RESET0(gNdsFighterEffectKindMask2);
    NDS_DIAG_RESET0(gNdsFighterEffectKindMask3);
    NDS_DIAG_RESET0(gNdsFighterModelPartSetCount);
    NDS_DIAG_RESET0(gNdsFighterModelPartOnCount);
    NDS_DIAG_RESET0(gNdsFighterModelPartResetCount);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofSpawnCallCount);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofSpawnSuccessCount);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofUpdateDestroyCount);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofMapDestroyCount);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofHitDestroyCount);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofWeaponFrames);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofWeaponCountMax);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofKindMask);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofAttackStateMask);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofDamageMax);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofLifetimeMax);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofMapMask);
    gNdsFighterProjectileProofFireballIndex = -1;
    NDS_DIAG_RESET0(gNdsFighterProjectileProofFireballInitialVelXMilli);
    NDS_DIAG_RESET0(gNdsFighterProjectileProofFireballInitialVelYMilli);
    ndsCollisionRuntimeDiagnosticsReset();
    NDS_DIAG_RESET0(gNdsFighterReflectorProofResult);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofMask);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFoxSlot);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofProjectileSlot);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofDownBPressFrames);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofStartFrames);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofLoopFrames);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofHitFrames);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofIsReflectFrames);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofReflectLRBeforeHit);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofReflectLRClearFrames);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofHitSetCallCount);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballProcCount);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballVelXBefore);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballVelXAfter);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballOwnerKind);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballCanReflect);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballCanAbsorb);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballCanShield);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballAttackCount);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballDamage);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballSizeMilli);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballDXMilli);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofFireballDYMilli);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofSpecialSizeMilli);
    NDS_DIAG_RESET0(gNdsFighterReflectorProofSpecialResist);
    NDS_DIAG_RESET0(gNdsFighterSpecialsProofMask);
    NDS_DIAG_RESET0(gNdsFighterSpecialsProofPhase);
    NDS_DIAG_RESET0(gNdsFighterSpecialsProofPhaseFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioSlot);
    NDS_DIAG_RESET0(gNdsFighterSpecialsFoxSlot);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioHiPressFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioHiFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioAirHiFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioFallSpecialFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioLandingFallSpecialFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioHiWaitFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioHiRootYMilli);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioHiDamageMax);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioLwPressFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioLwFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioAirLwFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioLwDustEffectCount);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioLwWaitFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsMarioLwDamageMax);
    NDS_DIAG_RESET0(gNdsFighterSpecialsFoxHiPressFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsFoxHiStartFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsFoxHiHoldFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsFoxHiTravelFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsFoxHiEndFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsFoxHiBoundFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsFoxHiWaitFrames);
    NDS_DIAG_RESET0(gNdsFighterSpecialsFoxHiRootYMilli);
#if NDS_P2_DONKEY
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsSlot)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNChargePressFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNStartFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNLoopFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNStorePressFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNStoredChargeMax)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNStoredWaitFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNResumePressFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNReleaseTapFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNEndFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNReleaseChargeMax)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNPassiveResetFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsNReleaseWaitFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsHiPressFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsHiFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsHiGroundGAFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsHiWaitFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsLwPressFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsLwStartFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsLwLoopFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsLwRepeatPressFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsLwLoopFlagFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsLwEndFrames)
            NDS_DIAG_WORD0(gNdsFighterDonkeySpecialsLwWaitFrames)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
#endif
#if NDS_P2_SAMUS
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsSlot);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsNPressFrames);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsNStartFrames);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsNLoopFrames);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsNChargeMax);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsNFullWaitFrames);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsNReleasePressFrames);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsNEndFrames);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsNReleaseWaitFrames);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsLwPressFrames);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsLwFrames);
    NDS_DIAG_RESET0(gNdsFighterSamusSpecialsLwWaitFrames);
#endif
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetMask);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetPhase);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetPhaseFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetTiltS3Frames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetTiltHi3Frames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetTiltLw3Frames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetTiltHitboxFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetSmashFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetSmashHitboxFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetAerialFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetAerialHitboxFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetLandingFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetCatchFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetCatchWaitFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetThrowFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetThrownFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetThrowRecoverFrames);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetThrowDamageBefore);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetThrowDamageAfter);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetAttackerStatus);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetAttackerMotion);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetAttackerGA);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetAttackerRootYMilli);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetVictimStatus);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetVictimMotion);
    NDS_DIAG_RESET0(gNdsFighterNaturalMovesetVictimGA);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterNaturalMovesetVictimRootYMilli)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableResult)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableMask)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableVictimSlot)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableVictimStockStart)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableVictimStockFinal)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableBattleStockStart)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableBattleStockFinal)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFallsStart)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFallsFinal)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableDeadFrames)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableRebirthDownFrames)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableRebirthStandFrames)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableRebirthWaitFrames)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFallAfterRebirthFrames)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableWaitAfterRebirthFrames)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalStatus)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalGA)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalFloor)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalIsRebirth)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalIsGhost)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalCameraMode)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableKOStickFrames)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableMapCallCount)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableMapHitCount)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableMapFloorHitCount)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableMapCliffHitCount)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableMapCeilHitCount)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableMapLastMaskStat)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableMapLastMaskCurr)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalXMilli)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalYMilli)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalVelXMilli)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalVelYMilli)
            NDS_DIAG_WORD0(gNdsFighterBattlePlayableFinalFloorDistMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFTComputerSetupCount);
    NDS_DIAG_RESET0(gNdsFTComputerDamageDetectCount);
    NDS_DIAG_RESET0(gNdsFTComputerProcessCount);
    NDS_DIAG_RESET0(gNdsFTComputerTargetFrames);
    NDS_DIAG_RESET0(gNdsFTComputerObjectiveMask);
    NDS_DIAG_RESET0(gNdsFTComputerBehaviorMask);
    NDS_DIAG_RESET0(gNdsFTComputerInputChangeCount);
    NDS_DIAG_RESET0(gNdsFTComputerStickFrames);
    NDS_DIAG_RESET0(gNdsFTComputerButtonAFrames);
    NDS_DIAG_RESET0(gNdsFTComputerButtonBFrames);
    NDS_DIAG_RESET0(gNdsFTComputerButtonZFrames);
    NDS_DIAG_RESET0(gNdsFTComputerAttackFrames);
    NDS_DIAG_RESET0(gNdsFTComputerHitboxFrames);
    NDS_DIAG_RESET0(gNdsFTComputerGuardFrames);
    NDS_DIAG_RESET0(gNdsFTComputerRecoveryFrames);
    NDS_DIAG_RESET0(gNdsFTComputerStatusChangeCount);
    NDS_DIAG_RESET0(gNdsFTComputerFinalStatus);
    NDS_DIAG_RESET0(gNdsFTComputerFinalGA);
    NDS_DIAG_RESET0(gNdsFTComputerFinalInputKind);
    NDS_DIAG_RESET0(gNdsFTComputerMarioDamageMax);
    NDS_DIAG_RESET0(gNdsFTComputerFloorLineCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFTComputerStartXMilli)
            NDS_DIAG_WORD0(gNdsFTComputerMinXMilli)
            NDS_DIAG_WORD0(gNdsFTComputerMaxXMilli)
            NDS_DIAG_WORD0(gNdsFTComputerFinalXMilli)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingResult)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingMode)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingLogicFrames)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPresentedFrames)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingDrawCalls)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingTimerTicks)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPresentFpsX10)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingLogicFpsX10)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingVBlankStart)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingVBlanks)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingRestartRequested)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPresentIntervalMin)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPresentIntervalMax)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPresentIntervalBucket[0])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPresentIntervalBucket[1])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPresentIntervalBucket[2])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPresentIntervalBucket[3])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPresentIntervalBucket[4])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPresentIntervalBucket[5])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingCadenceViolationCount)
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPhasePresentCount[0])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPhasePresentCount[1])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPhasePresentCount[2])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPhasePresentCount[3])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPhasePresentCount[4])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPhaseSlipCount[0])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPhaseSlipCount[1])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPhaseSlipCount[2])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPhaseSlipCount[3])
            NDS_DIAG_WORD0(gNdsBattlePlayablePacingPhaseSlipCount[4])
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
#if (NDS_HARNESS_FAST_LOGIC == 0) && \
    (NDS_RENDERER_HW_TRIANGLES != 0) && \
    (NDS_DEV_LIVE_INPUT_PREVIEW != 0)
    gNdsBuildModeCanonicalWord = NDS_BUILD_MODE_CANO_WORD;
    gNdsBuildModeShippedWord = NDS_BUILD_MODE_SHIP_WORD;
#else
    NDS_DIAG_RESET0(gNdsBuildModeCanonicalWord);
    NDS_DIAG_RESET0(gNdsBuildModeShippedWord);
#endif
#if NDS_HARNESS_FAST_LOGIC != 0
    gNdsBuildModeFastWord = NDS_BUILD_MODE_FAST_WORD;
#else
    NDS_DIAG_RESET0(gNdsBuildModeFastWord);
#endif
    NDS_DIAG_RESET0(gNdsRendererProfileFrameCount);
    NDS_DIAG_RESET0(gNdsRendererProfileUpdateTicks);
    NDS_DIAG_RESET0(gNdsRendererProfilePresentTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileDrawTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileHudTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileStageAdapterTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileMaterialTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileDLTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureConvertTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureUploadTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureUploads);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureUploadBytes);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureCi4DirectPixels);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureBinds);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureSourceTexels);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureGreenTexels);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureNonWhiteTexels);
    NDS_DIAG_RESET0(gNdsRendererProfileTexturedVertexCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureSampleCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureSampleGreenCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureSampleNonWhiteCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureCacheAliasAvoidCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLookupCallCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLookupProbeCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLookupActiveHitCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLookupTableHitCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLookupMissCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTexel1CompositeCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTexel1LoadMatchCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTexel1RejectCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTexel1RejectReasonMask);
    NDS_DIAG_RESET0(gNdsRendererProfileTexel1LastFraction);
    NDS_DIAG_RESET0(gNdsRendererProfileTexel1LastImage0);
    NDS_DIAG_RESET0(gNdsRendererProfileTexel1LastImage1);
    NDS_DIAG_RESET0(gNdsRendererProfileTexel1LastTileState);
    NDS_DIAG_RESET0(gNdsRendererProfileTexel1LastPrimaryState);
    NDS_DIAG_RESET0(gNdsRendererProfileTexel1FractionRefreshCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureCacheEvictCount);
    gNdsRendererProfileTextureCoordMinS = 32767;
    gNdsRendererProfileTextureCoordMaxS = -32768;
    gNdsRendererProfileTextureCoordMinT = 32767;
    gNdsRendererProfileTextureCoordMaxT = -32768;
    NDS_DIAG_RESET0(gNdsRendererProfileTextureConvertFormatMask);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureBindFormatMask);
    NDS_DIAG_RESET0(gNdsRendererProfileTexturePaletteFormatMask);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureRejectFormatMask);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureRejectReasonMask);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLaneLayoutMask);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLaneByteAccessCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLaneHalfwordAccessCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLaneByteFormatMask);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLaneHalfwordFormatMask);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLaneByteMap);
    NDS_DIAG_RESET0(gNdsRendererProfileTextureLaneHalfwordMap);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureRejectNoStatsCount);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureRejectStateOffCount);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureRejectNoCombineCount);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureRejectPrimitiveDecalCount);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureRejectNoTexel0Count);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureImplicitOnCount);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureRejectFirstReason);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureRejectFirstFlags);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureRejectFirstW0);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureRejectFirstW1);
    NDS_DIAG_RESET0(gNdsRendererProfileUseTextureRejectFirstGeometry);
    NDS_DIAG_RESET0(gNdsRendererProfileCombineModeCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCombineModeDistinctCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCombineMode0W0);
    NDS_DIAG_RESET0(gNdsRendererProfileCombineMode0W1);
    NDS_DIAG_RESET0(gNdsRendererProfileCombineMode1W0);
    NDS_DIAG_RESET0(gNdsRendererProfileCombineMode1W1);
    NDS_DIAG_RESET0(gNdsRendererProfileCombineMode2W0);
    NDS_DIAG_RESET0(gNdsRendererProfileCombineMode2W1);
    NDS_DIAG_RESET0(gNdsRendererProfileCombineMode3W0);
    NDS_DIAG_RESET0(gNdsRendererProfileCombineMode3W1);
    NDS_DIAG_RESET0(gNdsRendererProfileLitShadeCombineCount);
    NDS_DIAG_RESET0(gNdsRendererProfileMaterialCombineCount);
    NDS_DIAG_RESET0(gNdsRendererProfileProjectedSubmitFallbackCount);
    NDS_DIAG_RESET0(gNdsRendererProfileNearPlaneTriangleRejectCount);
    NDS_DIAG_RESET0(gNdsRendererProfileLightColorCommands);
    NDS_DIAG_RESET0(gNdsRendererProfileLightDirectionCommands);
    NDS_DIAG_RESET0(gNdsRendererProfileLightFallbackCount);
    NDS_DIAG_RESET0(gNdsRendererProfileHardwareVertices);
    NDS_DIAG_RESET0(gNdsRendererProfileHardwareTriangles);
    NDS_DIAG_RESET0(gNdsRendererProfileHardwareBatchBeginCount);
    NDS_DIAG_RESET0(gNdsRendererProfileHardwareBatchReuseCount);
    NDS_DIAG_RESET0(gNdsRendererProfileHardwareBatchEndCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTexturePrepareCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTexturePrepareReuseCount);
    NDS_DIAG_RESET0(gNdsRendererProfileImmutableListCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTrustedCommandCount);
    NDS_DIAG_RESET0(gNdsRendererProfileValidatedCommandCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTriangleRunReuseCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTriangleSubmitTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileVertexSubmitTicks);
    NDS_DIAG_RESET0(gNdsRendererProfileCi4LutBuildCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCi4LutReuseCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCi4IndexCacheBuildCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCi4IndexCacheReuseCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCi4RepresentativePixelCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCi4ReusePixelCount);
    NDS_DIAG_RESET0(gNdsRendererProfileHardwareOverLimit);
    NDS_DIAG_RESET0(gNdsRendererProfileOracleSamples);
    NDS_DIAG_RESET0(gNdsRendererProfileOracleMismatches);
    NDS_DIAG_RESET0(gNdsRendererProfileOracleMaxDelta);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixLoadCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCameraMatrixCacheHitCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCameraMatrixCacheMissCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCameraMatrixCacheOverflowCount);
    NDS_DIAG_RESET0(gNdsRendererProfileDObjWorldCacheHitCount);
    NDS_DIAG_RESET0(gNdsRendererProfileDObjWorldCacheMissCount);
    NDS_DIAG_RESET0(gNdsRendererProfileDObjWorldCacheOverflowCount);
    NDS_DIAG_RESET0(gNdsRendererProfileStageWorldPersistentHitCount);
    NDS_DIAG_RESET0(gNdsRendererProfileStageWorldPersistentMissCount);
    NDS_DIAG_RESET0(gNdsRendererProfileStageWorldPersistentRejectCount);
    NDS_DIAG_RESET0(gNdsRendererProfileStageWorldPersistentOverflowCount);
    NDS_DIAG_RESET0(gNdsRendererProfileStageWorldPersistentOracleSampleCount);
    NDS_DIAG_RESET0(gNdsRendererProfileStageWorldPersistentOracleMismatchCount);
    NDS_DIAG_RESET0(gNdsRendererProfileAffineMatrixSamples);
    NDS_DIAG_RESET0(gNdsRendererProfileAffineMatrixMismatches);
    NDS_DIAG_RESET0(gNdsRendererProfileAffineMatrixMaxDelta);
    NDS_DIAG_RESET0(gNdsRendererProfileRawCurrentCandidateCount);
    NDS_DIAG_RESET0(gNdsRendererProfileRawCurrentRangeRejectCount);
    NDS_DIAG_RESET0(gNdsRendererProfileRawCrossMatrixCount);
    NDS_DIAG_RESET0(gNdsRendererProfileSubmitRawCurrentCount);
    NDS_DIAG_RESET0(gNdsRendererProfileSubmitRawSnapshotCount);
    NDS_DIAG_RESET0(gNdsRendererProfileSubmitProjectedCrossCount);
    NDS_DIAG_RESET0(gNdsRendererProfileSubmitProjectedNoZCount);
    NDS_DIAG_RESET0(gNdsRendererProfileSubmitProjectedDecalCount);
    NDS_DIAG_RESET0(gNdsRendererProfileSubmitProjectedPrimDepthCount);
    NDS_DIAG_RESET0(gNdsRendererProfileSubmitProjectedRangeOrMatrixCount);
    NDS_DIAG_RESET0(gNdsRendererProfileSubmitRejectCount);
    NDS_DIAG_RESET0(gNdsRendererProfileHardwareDivideSummary);
    NDS_DIAG_RESET0(gNdsRendererProfileSourceVertexLoadCount);
    NDS_DIAG_RESET0(gNdsRendererProfileCPUTransformCount);
    NDS_DIAG_RESET0(gNdsRendererProfileTransformCacheHitCount);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixSnapshotCreateCount);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixSnapshotReuseCount);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixSnapshotOverflowCount);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixPosTestSamples);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixPosTestMismatches);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixPosTestMaxError);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixPosTestWSignMismatches);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixPosTestClipMismatches);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixPosTestMatrixWordSamples);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixPosTestDropped);
    NDS_DIAG_RESET0(gNdsRendererProfileMatrixScaleWorld);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsRendererProfileProjectionM00)
            NDS_DIAG_WORD0(gNdsRendererProfileProjectionM11)
            NDS_DIAG_WORD0(gNdsRendererProfileProjectionM22)
            NDS_DIAG_WORD0(gNdsRendererProfileProjectionM32)
            NDS_DIAG_WORD0(gNdsRendererProfileModelviewM00)
            NDS_DIAG_WORD0(gNdsRendererProfileModelviewM11)
            NDS_DIAG_WORD0(gNdsRendererProfileModelviewM22)
            NDS_DIAG_WORD0(gNdsRendererProfileModelviewM30)
            NDS_DIAG_WORD0(gNdsRendererProfileModelviewM31)
            NDS_DIAG_WORD0(gNdsRendererProfileModelviewM32)
            NDS_DIAG_WORD0(gNdsRendererProfileRawVertexMinX)
            NDS_DIAG_WORD0(gNdsRendererProfileRawVertexMaxX)
            NDS_DIAG_WORD0(gNdsRendererProfileRawVertexMinY)
            NDS_DIAG_WORD0(gNdsRendererProfileRawVertexMaxY)
            NDS_DIAG_WORD0(gNdsRendererProfileRawVertexMinZ)
            NDS_DIAG_WORD0(gNdsRendererProfileRawVertexMaxZ)
            NDS_DIAG_WORD0(gNdsRendererProfileHWVertexMinX)
            NDS_DIAG_WORD0(gNdsRendererProfileHWVertexMaxX)
            NDS_DIAG_WORD0(gNdsRendererProfileHWVertexMinY)
            NDS_DIAG_WORD0(gNdsRendererProfileHWVertexMaxY)
            NDS_DIAG_WORD0(gNdsRendererProfileHWVertexMinZ)
            NDS_DIAG_WORD0(gNdsRendererProfileHWVertexMaxZ)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsRendererProfileHWVertexSaturateCount);
    NDS_DIAG_RESET0(gNdsRendererDepthStageSamples);
    NDS_DIAG_RESET0(gNdsRendererDepthStageMin);
    NDS_DIAG_RESET0(gNdsRendererDepthStageMax);
    NDS_DIAG_RESET0(gNdsRendererDepthStageWMin);
    NDS_DIAG_RESET0(gNdsRendererDepthStageWMax);
    NDS_DIAG_RESET0(gNdsRendererDepthFighterP0Samples);
    NDS_DIAG_RESET0(gNdsRendererDepthFighterP0Min);
    NDS_DIAG_RESET0(gNdsRendererDepthFighterP0Max);
    NDS_DIAG_RESET0(gNdsRendererDepthFighterP0WMin);
    NDS_DIAG_RESET0(gNdsRendererDepthFighterP0WMax);
    NDS_DIAG_RESET0(gNdsRendererDepthFighterP1Samples);
    NDS_DIAG_RESET0(gNdsRendererDepthFighterP1Min);
    NDS_DIAG_RESET0(gNdsRendererDepthFighterP1Max);
    NDS_DIAG_RESET0(gNdsRendererDepthFighterP1WMin);
    NDS_DIAG_RESET0(gNdsRendererDepthFighterP1WMax);
    NDS_DIAG_RESET0(gNdsIFCommonHUDRecordCount);
    NDS_DIAG_RESET0(gNdsIFCommonHUDObjectMask);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0DamageCurrent);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1DamageCurrent);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2DamageCurrent);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3DamageCurrent);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0DamageMax);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1DamageMax);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2DamageMax);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3DamageMax);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0DigitCount);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1DigitCount);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2DigitCount);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3DigitCount);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0Digits);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1Digits);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2Digits);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3Digits);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0StockCurrent);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1StockCurrent);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2StockCurrent);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3StockCurrent);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0StockMin);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1StockMin);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2StockMin);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3StockMin);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0StockMax);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1StockMax);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2StockMax);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3StockMax);
    NDS_DIAG_RESET0(gNdsIFCommonHUDActivePlayerMask);
    NDS_DIAG_RESET0(gNdsIFCommonHUDShowDamageMask);
    NDS_DIAG_RESET0(gNdsIFCommonHUDDamageFlashMask);
    NDS_DIAG_RESET0(gNdsIFCommonHUDSingleStockMask);
    NDS_DIAG_RESET0(gNdsIFCommonHUDCPUPlayerMask);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0FighterKind);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1FighterKind);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2FighterKind);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3FighterKind);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0Level);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1Level);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2Level);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3Level);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0Costume);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1Costume);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2Costume);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3Costume);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP0LowerStock);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP1LowerStock);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP2LowerStock);
    NDS_DIAG_RESET0(gNdsIFCommonHUDP3LowerStock);
    NDS_DIAG_RESET0(gNdsIFCommonHUDTimeRemain);
    NDS_DIAG_RESET0(gNdsIFCommonHUDTimerLimit);
    NDS_DIAG_RESET0(gNdsIFCommonHUDTimerStarted);
    NDS_DIAG_RESET0(gNdsIFCommonHUDTimerVisible);
    NDS_DIAG_RESET0(gNdsIFCommonHUDGameStatus);
    NDS_DIAG_RESET0(gNdsIFCommonHUDLowerRouteMask);
    NDS_DIAG_RESET0(gNdsIFCommonHUDLowerRouteCount);
    NDS_DIAG_RESET0(gNdsIFCommonHUDLowerTimerRouteCount);
    NDS_DIAG_RESET0(gNdsIFCommonHUDLowerStockRouteCount);
    NDS_DIAG_RESET0(gNdsIFCommonHUDLowerDamageRouteCount);
    NDS_DIAG_RESET0(gNdsIFCommonHUDTopGenericPassCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxModelResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGObjResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxSetupMask);
    NDS_DIAG_RESET0(gNdsFighterMarioMainMotionPtrReady);
    NDS_DIAG_RESET0(gNdsFighterMarioMainPtrReady);
    NDS_DIAG_RESET0(gNdsFighterMarioModelPtrReady);
    NDS_DIAG_RESET0(gNdsFighterMarioAttrPtrReady);
    NDS_DIAG_RESET0(gNdsFighterMarioCommonPartsReady);
    NDS_DIAG_RESET0(gNdsFighterFoxMainMotionPtrReady);
    NDS_DIAG_RESET0(gNdsFighterFoxMainPtrReady);
    NDS_DIAG_RESET0(gNdsFighterFoxModelPtrReady);
    NDS_DIAG_RESET0(gNdsFighterFoxAttrPtrReady);
    NDS_DIAG_RESET0(gNdsFighterFoxCommonPartsReady);
    NDS_DIAG_RESET0(gNdsFighterModelRealGObjCount);
    NDS_DIAG_RESET0(gNdsFighterModelStubGObjCount);
    NDS_DIAG_RESET0(gNdsFighterModelProcessDeferredCount);
    gNdsFighterModelP0FKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterModelP0GObjID);
    NDS_DIAG_RESET0(gNdsFighterModelP0TopDObjReady);
    NDS_DIAG_RESET0(gNdsFighterModelP0ModelDObjCount);
    NDS_DIAG_RESET0(gNdsFighterModelP0MObjCount);
    NDS_DIAG_RESET0(gNdsFighterModelP0AObjCount);
    NDS_DIAG_RESET0(gNdsFighterModelP0DisplayAttached);
    gNdsFighterModelP1FKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterModelP1GObjID);
    NDS_DIAG_RESET0(gNdsFighterModelP1TopDObjReady);
    NDS_DIAG_RESET0(gNdsFighterModelP1ModelDObjCount);
    NDS_DIAG_RESET0(gNdsFighterModelP1MObjCount);
    NDS_DIAG_RESET0(gNdsFighterModelP1AObjCount);
    NDS_DIAG_RESET0(gNdsFighterModelP1DisplayAttached);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStructResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxJointResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStateResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStructMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStructPoolUsedMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStructCount);
    NDS_DIAG_RESET0(gNdsFighterStructP0PtrReady);
    NDS_DIAG_RESET0(gNdsFighterStructP1PtrReady);
    NDS_DIAG_RESET0(gNdsFighterStructP0UserDataPtrReady);
    NDS_DIAG_RESET0(gNdsFighterStructP1UserDataPtrReady);
    NDS_DIAG_RESET0(gNdsFighterStructP0FtGetStructReady);
    NDS_DIAG_RESET0(gNdsFighterStructP1FtGetStructReady);
    gNdsFighterStructP0FKind = 0xffffffffu;
    gNdsFighterStructP1FKind = 0xffffffffu;
    gNdsFighterStructP0PKind = 0xffffffffu;
    gNdsFighterStructP1PKind = 0xffffffffu;
    gNdsFighterStructP0Player = 0xffffffffu;
    gNdsFighterStructP1Player = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterStructP0LR);
    NDS_DIAG_RESET0(gNdsFighterStructP1LR);
    NDS_DIAG_RESET0(gNdsFighterStructP0Stock);
    NDS_DIAG_RESET0(gNdsFighterStructP1Stock);
    NDS_DIAG_RESET0(gNdsFighterStructP0Detail);
    NDS_DIAG_RESET0(gNdsFighterStructP1Detail);
    NDS_DIAG_RESET0(gNdsFighterStructP0Costume);
    NDS_DIAG_RESET0(gNdsFighterStructP1Costume);
    NDS_DIAG_RESET0(gNdsFighterStructP0Shade);
    NDS_DIAG_RESET0(gNdsFighterStructP1Shade);
    NDS_DIAG_RESET0(gNdsFighterStructP0AttrReady);
    NDS_DIAG_RESET0(gNdsFighterStructP1AttrReady);
    NDS_DIAG_RESET0(gNdsFighterStructP0FigatreeHeapReady);
    NDS_DIAG_RESET0(gNdsFighterStructP1FigatreeHeapReady);
    NDS_DIAG_RESET0(gNdsFighterStructP0ControllerReady);
    NDS_DIAG_RESET0(gNdsFighterStructP1ControllerReady);
    NDS_DIAG_RESET0(gNdsFighterStructP0InputMaskA);
    NDS_DIAG_RESET0(gNdsFighterStructP1InputMaskA);
    NDS_DIAG_RESET0(gNdsFighterStructP0InputMaskB);
    NDS_DIAG_RESET0(gNdsFighterStructP1InputMaskB);
    NDS_DIAG_RESET0(gNdsFighterStructP0InputMaskZ);
    NDS_DIAG_RESET0(gNdsFighterStructP1InputMaskZ);
    NDS_DIAG_RESET0(gNdsFighterStructP0InputMaskL);
    NDS_DIAG_RESET0(gNdsFighterStructP1InputMaskL);
    NDS_DIAG_RESET0(gNdsFighterStructP0TopJointReady);
    NDS_DIAG_RESET0(gNdsFighterStructP1TopJointReady);
    NDS_DIAG_RESET0(gNdsFighterStructP0JointCount);
    NDS_DIAG_RESET0(gNdsFighterStructP1JointCount);
    NDS_DIAG_RESET0(gNdsFighterStructP0CommonJointCount);
    NDS_DIAG_RESET0(gNdsFighterStructP1CommonJointCount);
    NDS_DIAG_RESET0(gNdsFighterStructP0CollTranslateReady);
    NDS_DIAG_RESET0(gNdsFighterStructP1CollTranslateReady);
    NDS_DIAG_RESET0(gNdsFighterStructP0CollLRReady);
    NDS_DIAG_RESET0(gNdsFighterStructP1CollLRReady);
    NDS_DIAG_RESET0(gNdsFighterStructP0StatusID);
    NDS_DIAG_RESET0(gNdsFighterStructP1StatusID);
    NDS_DIAG_RESET0(gNdsFighterStructP0StatusTotalTics);
    NDS_DIAG_RESET0(gNdsFighterStructP1StatusTotalTics);
    NDS_DIAG_RESET0(gNdsFighterStructProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterStructStatusSetCount);
    NDS_DIAG_RESET0(gNdsFighterStructDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxInitResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxCollResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDeferResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxInitMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxInitDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxInitCount);
    gNdsFighterInitP0FKind = 0xffffffffu;
    gNdsFighterInitP1FKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterInitP0PercentDamage);
    NDS_DIAG_RESET0(gNdsFighterInitP1PercentDamage);
    NDS_DIAG_RESET0(gNdsFighterInitP0ShieldHealth);
    NDS_DIAG_RESET0(gNdsFighterInitP1ShieldHealth);
    gNdsFighterInitP0GA = 0xffffffffu;
    gNdsFighterInitP1GA = 0xffffffffu;
    gNdsFighterInitP0JumpsUsed = 0xffffffffu;
    gNdsFighterInitP1JumpsUsed = 0xffffffffu;
    gNdsFighterInitP0HitStatus = 0xffffffffu;
    gNdsFighterInitP1HitStatus = 0xffffffffu;
    gNdsFighterInitP0DamageKind = 0xffffffffu;
    gNdsFighterInitP1DamageKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterInitP0MotionAttackID);
    NDS_DIAG_RESET0(gNdsFighterInitP1MotionAttackID);
    NDS_DIAG_RESET0(gNdsFighterInitP0FloorProjectAttempt);
    NDS_DIAG_RESET0(gNdsFighterInitP1FloorProjectAttempt);
    NDS_DIAG_RESET0(gNdsFighterInitP0FloorProjectResult);
    NDS_DIAG_RESET0(gNdsFighterInitP1FloorProjectResult);
    gNdsFighterInitP0FloorLineID = 0xffffffffu;
    gNdsFighterInitP1FloorLineID = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterInitP0FloorDistBits);
    NDS_DIAG_RESET0(gNdsFighterInitP1FloorDistBits);
    NDS_DIAG_RESET0(gNdsFighterInitP0RootTranslateXBits);
    NDS_DIAG_RESET0(gNdsFighterInitP1RootTranslateXBits);
    NDS_DIAG_RESET0(gNdsFighterInitP0RootTranslateYBits);
    NDS_DIAG_RESET0(gNdsFighterInitP1RootTranslateYBits);
    NDS_DIAG_RESET0(gNdsFighterInitP0RootScaleXBits);
    NDS_DIAG_RESET0(gNdsFighterInitP1RootScaleXBits);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterInitDamageCollMask)
            NDS_DIAG_WORD0(gNdsFighterInitDamageCollNormalMask)
            NDS_DIAG_WORD0(gNdsFighterInitDamageCollJointMask)
            NDS_DIAG_WORD0(gNdsFighterInitDamageCollHalfSizeMask)
            NDS_DIAG_WORD0(gNdsFighterInitDamageCollPartsMask)
            NDS_DIAG_WORD0(gNdsFighterInitDamageCollMatrixMask)
            NDS_DIAG_WORD0(gNdsFighterInitDamageCollScaleMask)
            NDS_DIAG_WORD0(gNdsFighterInitP0DamageCollCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterInitP1DamageCollCount);
    gNdsFighterInitP0DamageCollJoint0 = 0xffffffffu;
    gNdsFighterInitP1DamageCollJoint0 = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterInitP0DamageCollSizeXBits);
    NDS_DIAG_RESET0(gNdsFighterInitP1DamageCollSizeXBits);
    NDS_DIAG_RESET0(gNdsFighterInitP0DamageCollSizeYBits);
    NDS_DIAG_RESET0(gNdsFighterInitP1DamageCollSizeYBits);
    NDS_DIAG_RESET0(gNdsFighterInitP0DamageCollSizeZBits);
    NDS_DIAG_RESET0(gNdsFighterInitP1DamageCollSizeZBits);
    NDS_DIAG_RESET0(gNdsFighterInitP0DamageCollWorldXBits);
    NDS_DIAG_RESET0(gNdsFighterInitP1DamageCollWorldXBits);
    NDS_DIAG_RESET0(gNdsFighterInitP0DamageCollWorldYBits);
    NDS_DIAG_RESET0(gNdsFighterInitP1DamageCollWorldYBits);
    NDS_DIAG_RESET0(gNdsFighterInitP0DamageCollScaleXBits);
    NDS_DIAG_RESET0(gNdsFighterInitP1DamageCollScaleXBits);
    NDS_DIAG_RESET0(gNdsFighterInitPhysicsStopCount);
    NDS_DIAG_RESET0(gNdsFighterInitAttackClearCount);
    NDS_DIAG_RESET0(gNdsFighterInitHitStatusPartCount);
    NDS_DIAG_RESET0(gNdsFighterInitColAnimResetCount);
    gNdsFighterInitP0PassiveMarioTornado = 0xffffffffu;
    gNdsFighterInitP1PassiveMarioTornado = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterInitP0ThrowCatchItemClear);
    NDS_DIAG_RESET0(gNdsFighterInitP1ThrowCatchItemClear);
    NDS_DIAG_RESET0(gNdsFighterInitProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterInitStatusSetCount);
    NDS_DIAG_RESET0(gNdsFighterInitDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitStatusResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitMotionResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitDeferResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitCount);
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
    NDS_DIAG_RESET0(gNdsFighterWaitP0AnimFrameBits);
    NDS_DIAG_RESET0(gNdsFighterWaitP1AnimFrameBits);
    NDS_DIAG_RESET0(gNdsFighterWaitP0AnimSpeedBits);
    NDS_DIAG_RESET0(gNdsFighterWaitP1AnimSpeedBits);
    NDS_DIAG_RESET0(gNdsFighterWaitP0SpecialInterrupt);
    NDS_DIAG_RESET0(gNdsFighterWaitP1SpecialInterrupt);
    NDS_DIAG_RESET0(gNdsFighterWaitP0PlayerTagWait);
    NDS_DIAG_RESET0(gNdsFighterWaitP1PlayerTagWait);
    NDS_DIAG_RESET0(gNdsFighterWaitP0ProcInterruptReady);
    NDS_DIAG_RESET0(gNdsFighterWaitP1ProcInterruptReady);
    NDS_DIAG_RESET0(gNdsFighterWaitP0ProcPhysicsReady);
    NDS_DIAG_RESET0(gNdsFighterWaitP1ProcPhysicsReady);
    NDS_DIAG_RESET0(gNdsFighterWaitP0ProcMapReady);
    NDS_DIAG_RESET0(gNdsFighterWaitP1ProcMapReady);
    NDS_DIAG_RESET0(gNdsFighterWaitP0MainMotionReady);
    NDS_DIAG_RESET0(gNdsFighterWaitP1MainMotionReady);
    gNdsFighterWaitP0GA = 0xffffffffu;
    gNdsFighterWaitP1GA = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterWaitFtMainSetStatusCallCount);
    NDS_DIAG_RESET0(gNdsFighterWaitOriginalSetStatusCallCount);
    NDS_DIAG_RESET0(gNdsFighterWaitHammerCheckCount);
    NDS_DIAG_RESET0(gNdsFighterWaitHammerDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundSetCount);
    NDS_DIAG_RESET0(gNdsFighterWaitPlayerTagSetCount);
    NDS_DIAG_RESET0(gNdsFighterWaitProcInterruptCallCount);
    NDS_DIAG_RESET0(gNdsFighterWaitProcPhysicsCallCount);
    NDS_DIAG_RESET0(gNdsFighterWaitProcMapCallCount);
    NDS_DIAG_RESET0(gNdsFighterWaitProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterWaitDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGameplayUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitTickResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitCallbackResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitTickMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitTickDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWaitTickCount);
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
    NDS_DIAG_RESET0(gNdsFighterWaitTickP0RootXBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP1RootXBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP0RootYBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP1RootYBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP0RootXAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP1RootXAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP0RootYAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP1RootYAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP0VelGroundXBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP1VelGroundXBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP0VelGroundXAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickP1VelGroundXAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWaitTickGObjCountBefore);
    NDS_DIAG_RESET0(gNdsFighterWaitTickGObjCountAfter);
    NDS_DIAG_RESET0(gNdsFighterWaitTickStatusChangeCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickMotionChangeCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickGADriftCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickRootDriftCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickGObjDelta);
    NDS_DIAG_RESET0(gNdsFighterWaitTickOriginalInterruptCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickGroundInterruptCheckCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickPhysicsCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickMapCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickDeniedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterWaitTickGameplayUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGroundPhysResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGroundMapResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGroundSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGroundMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGroundDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGroundCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0VelBeforeMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1VelBeforeMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0VelAfterMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1VelAfterMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0AirVelXMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1AirVelXMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0AirVelYMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1AirVelYMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0FrictionMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1FrictionMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0Material);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1Material);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0TractionMilli);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1TractionMilli);
    gNdsFighterWaitGroundP0StatusAfter = 0xffffffffu;
    gNdsFighterWaitGroundP1StatusAfter = 0xffffffffu;
    gNdsFighterWaitGroundP0MotionAfter = 0xffffffffu;
    gNdsFighterWaitGroundP1MotionAfter = 0xffffffffu;
    gNdsFighterWaitGroundP0GAAfter = 0xffffffffu;
    gNdsFighterWaitGroundP1GAAfter = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0RootXBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1RootXBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0RootXAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1RootXAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0RootYBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1RootYBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP0RootYAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundP1RootYAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundPhysicsCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundMapCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundMapCheckCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundMapSafeFloorCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundMapFallDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundMapOttottoDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundStatusChangeCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundMotionChangeCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundGADriftCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundRootDriftCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundGObjDelta);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterWaitGroundGameplayUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDisplayResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDisplaySafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDisplayMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDisplayDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDisplayCallbackCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDisplayP0DObjCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayP1DObjCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayP0MObjCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayP1MObjCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayP0AObjCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayP1AObjCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayP0DLReadyCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayP1DLReadyCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayP0PartsPtrCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayP1PartsPtrCount)
            NDS_DIAG_WORD(gNdsFighterDisplayP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1GAAfter, 1u),
            NDS_DIAG_WORD0(gNdsFighterDisplayP0RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDisplayP1RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDisplayP0RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDisplayP1RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDisplayGObjDelta)
            NDS_DIAG_WORD0(gNdsFighterDisplayDrawCallCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayMatrixCallCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayGameplayUpdateCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLScanResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLScanSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLScanMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLScanDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLScanCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDLScanP0FirstDL)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1FirstDL)
            NDS_DIAG_WORD(gNdsFighterDLScanP0AssetID, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1AssetID, 1u),
            NDS_DIAG_WORD0(gNdsFighterDLScanP0Offset)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1Offset)
            NDS_DIAG_WORD(gNdsFighterDLScanP0DObjIndex, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1DObjIndex, 1u),
            NDS_DIAG_WORD0(gNdsFighterDLScanP0Blocker)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1Blocker)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0CommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1CommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0FirstOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1FirstOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0UnsupportedOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1UnsupportedOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0UnsupportedCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1UnsupportedCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0VertexCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1VertexCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0TriangleCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1TriangleCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0VertexCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1VertexCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0TriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1TriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0EndCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1EndCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0BranchCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1BranchCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0SegmentResolveCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1SegmentResolveCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0TextureMask)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1TextureMask)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0OtherModeCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1OtherModeCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0CullCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1CullCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0StateCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1StateCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0SkipCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1SkipCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0RenderCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1RenderCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0MaxDepthSeen)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1MaxDepthSeen)
            NDS_DIAG_WORD(gNdsFighterDLScanP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1GAAfter, 1u),
            NDS_DIAG_WORD0(gNdsFighterDLScanP0RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDLScanP0RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDLScanP1RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDLScanGObjDelta)
            NDS_DIAG_WORD0(gNdsFighterDLScanDrawCallCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanMatrixCallCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanGameplayUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanRangeRejectCount)
            NDS_DIAG_WORD0(gNdsFighterDLScanBranchResolveCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLExecResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLExecSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLExecMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLExecDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLExecCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDLExecP0Blocker)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1Blocker)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0CommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1CommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0FirstOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1FirstOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0UnsupportedOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1UnsupportedOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0UnsupportedCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1UnsupportedCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0VertexCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1VertexCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0VertexDecodedCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1VertexDecodedCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0VertexValidMask)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1VertexValidMask)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0TriangleCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1TriangleCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0TriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1TriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0TriangleValidCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1TriangleValidCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0MinX)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0MaxX)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0MinY)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0MaxY)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0MinZ)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0MaxZ)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1MinX)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1MaxX)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1MinY)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1MaxY)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1MinZ)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1MaxZ)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0ColorChecksum)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1ColorChecksum)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0OtherModeCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1OtherModeCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0CullCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1CullCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0StateCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1StateCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0SkipCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1SkipCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0RenderCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1RenderCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0BranchCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1BranchCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0SegmentResolveCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1SegmentResolveCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0TextureMask)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1TextureMask)
            NDS_DIAG_WORD(gNdsFighterDLExecP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1GAAfter, 1u),
            NDS_DIAG_WORD0(gNdsFighterDLExecP0RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDLExecP0RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDLExecP1RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDLExecGObjDelta)
            NDS_DIAG_WORD0(gNdsFighterDLExecDrawCallCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecMatrixCallCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecGameplayUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecRangeRejectCount)
            NDS_DIAG_WORD0(gNdsFighterDLExecVertexRangeRejectCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLDrawResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLDrawSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLDrawMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLDrawDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLDrawCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDLDrawPreviewWidth)
            NDS_DIAG_WORD0(gNdsFighterDLDrawPreviewHeight)
            NDS_DIAG_WORD0(gNdsFighterDLDrawPreviewPitch)
            NDS_DIAG_WORD0(gNdsFighterDLDrawPreviewReady)
            NDS_DIAG_WORD0(gNdsFighterDLDrawPreviewCommitBefore)
            NDS_DIAG_WORD0(gNdsFighterDLDrawPreviewCommitAfter)
            NDS_DIAG_WORD0(gNdsFighterDLDrawPreviewCommitDelta)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0Blocker)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1Blocker)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0CommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1CommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0FirstOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1FirstOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0UnsupportedOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1UnsupportedOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0UnsupportedCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1UnsupportedCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0VertexDecodedCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1VertexDecodedCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0TriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1TriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0TriangleValidCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1TriangleValidCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0TriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1TriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0RealTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1RealTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0MarkerTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1MarkerTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0PixelCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1PixelCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawTotalPixelCount)
            NDS_DIAG_WORD(gNdsFighterDLDrawP0Axis, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1Axis, 1u),
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0Area)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1Area)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0MinA)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0MaxA)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0MinB)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0MaxB)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1MinA)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1MaxA)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1MinB)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1MaxB)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0ScreenMinX)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0ScreenMaxX)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0ScreenMinY)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0ScreenMaxY)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1ScreenMinX)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1ScreenMaxX)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1ScreenMinY)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1ScreenMaxY)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0ColorChecksum)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1ColorChecksum)
            NDS_DIAG_WORD(gNdsFighterDLDrawP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1GAAfter, 1u),
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP0RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDLDrawP1RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDLDrawGObjDelta)
            NDS_DIAG_WORD0(gNdsFighterDLDrawDrawCallCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawMatrixCallCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawGameplayUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawRangeRejectCount)
            NDS_DIAG_WORD0(gNdsFighterDLDrawVertexRangeRejectCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLMultiDrawResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLMultiDrawSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLMultiDrawMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLMultiDrawDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLMultiDrawCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawPreviewWidth)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawPreviewHeight)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawPreviewPitch)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawPreviewReady)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawPreviewCommitBefore)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawPreviewCommitAfter)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawPreviewCommitDelta)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0CandidateCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1CandidateCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0SelectedCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1SelectedCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0AttemptCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1AttemptCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0CleanCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1CleanCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0DrawnDObjCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1DrawnDObjCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0FailedCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1FailedCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0SelectedIndexMask)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1SelectedIndexMask)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0FirstBlocker)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1FirstBlocker)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0BlockerMask)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1BlockerMask)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0CommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1CommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0FirstOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1FirstOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0UnsupportedOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1UnsupportedOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0UnsupportedCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1UnsupportedCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0VertexDecodedCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1VertexDecodedCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0TriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1TriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0TriangleValidCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1TriangleValidCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0TriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1TriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0RealTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1RealTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0MarkerTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1MarkerTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0PixelCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1PixelCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawTotalPixelCount)
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0Axis, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1Axis, 1u),
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0Area)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1Area)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0MinA)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0MaxA)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0MinB)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0MaxB)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1MinA)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1MaxA)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1MinB)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1MaxB)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0ScreenMinX)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0ScreenMaxX)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0ScreenMinY)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0ScreenMaxY)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1ScreenMinX)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1ScreenMaxX)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1ScreenMinY)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1ScreenMaxY)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0ColorChecksum)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1ColorChecksum)
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1GAAfter, 1u),
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP0RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawP1RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawGObjDelta)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawDrawCallCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawMatrixCallCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawGameplayUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawRangeRejectCount)
            NDS_DIAG_WORD0(gNdsFighterDLMultiDrawVertexRangeRejectCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLAllDrawResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLAllDrawSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLAllDrawMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLAllDrawDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDLAllDrawCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawDisplayCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0DisplayCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1DisplayCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawPreviewWidth)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawPreviewHeight)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawPreviewPitch)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawPreviewReady)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawPreviewCommitBefore)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawPreviewCommitAfter)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawPreviewCommitDelta)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0CandidateCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1CandidateCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0SelectedCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1SelectedCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawCandidateHighWater)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawSelectedHighWater)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawTruncateCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawSelectedOverflowCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFTManagerFigatreeSlotKindCount);
    NDS_DIAG_RESET0(gNdsFTManagerFigatreeSlotKindBytes);
    NDS_DIAG_RESET0(gNdsFTManagerFigatreeSlotKindMin);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0AttemptCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1AttemptCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0CleanCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1CleanCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0DrawnDObjCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1DrawnDObjCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0FailedCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1FailedCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0SelectedIndexMask)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1SelectedIndexMask)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0FirstBlocker)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1FirstBlocker)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0BlockerMask)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1BlockerMask)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0CommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1CommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0FirstOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1FirstOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0UnsupportedOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1UnsupportedOpcode)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0UnsupportedCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1UnsupportedCommandCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0VertexDecodedCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1VertexDecodedCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0MatrixMvpRecalcCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1MatrixMvpRecalcCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0MatrixMoveWordCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1MatrixMoveWordCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0HardwareTriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawSlotTriangleMask)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1HardwareTriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0HardwareOracleTriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1HardwareOracleTriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0HardwareOracleRejectCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1HardwareOracleRejectCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0HardwareMatrixSeedCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1HardwareMatrixSeedCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawHardwareTextureBindCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawHardwareTextureUploadCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawHardwareTextureReadyCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawHardwareTextureRejectCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawHardwareTextureFormatMask)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawHardwareTextureMaxWidth)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawHardwareTextureMaxHeight)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0TriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1TriangleCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0TriangleValidCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1TriangleValidCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0TriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1TriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0RealTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1RealTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0MarkerTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1MarkerTriangleDrawnCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0PixelCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1PixelCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawTotalPixelCount)
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0Axis, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1Axis, 1u),
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0Area)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1Area)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0MinA)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0MaxA)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0MinB)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0MaxB)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1MinA)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1MaxA)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1MinB)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1MaxB)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0ScreenMinX)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0ScreenMaxX)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0ScreenMinY)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0ScreenMaxY)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1ScreenMinX)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1ScreenMaxX)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1ScreenMinY)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1ScreenMaxY)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0ColorChecksum)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1ColorChecksum)
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1GAAfter, 1u),
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP0RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1RootXBeforeBits)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawP1RootXAfterBits)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawGObjDelta)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawDrawCallCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawMatrixCallCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawGameplayUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawRangeRejectCount)
            NDS_DIAG_WORD0(gNdsFighterDLAllDrawVertexRangeRejectCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWalkInputResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWalkSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWalkInputMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWalkDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWalkInputCount);
    NDS_DIAG_RESET0(gNdsFighterWalkP0StickX);
    NDS_DIAG_RESET0(gNdsFighterWalkP1StickX);
    NDS_DIAG_RESET0(gNdsFighterWalkP0StickAbs);
    NDS_DIAG_RESET0(gNdsFighterWalkP1StickAbs);
    NDS_DIAG_RESET0(gNdsFighterWalkP0LR);
    NDS_DIAG_RESET0(gNdsFighterWalkP1LR);
    NDS_DIAG_RESET0(gNdsFighterWalkP0InputSuccess);
    NDS_DIAG_RESET0(gNdsFighterWalkP1InputSuccess);
    NDS_DIAG_RESET0(gNdsFighterWalkP0SelectedStatus);
    NDS_DIAG_RESET0(gNdsFighterWalkP1SelectedStatus);
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
    NDS_DIAG_RESET0(gNdsFighterWalkWaitInterruptCallCount);
    NDS_DIAG_RESET0(gNdsFighterWalkGroundCheckCallCount);
    NDS_DIAG_RESET0(gNdsFighterWalkOriginalCheckCallCount);
    NDS_DIAG_RESET0(gNdsFighterWalkOriginalCheckSuccessCount);
    NDS_DIAG_RESET0(gNdsFighterWalkSetStatusCallCount);
    NDS_DIAG_RESET0(gNdsFighterWalkFtMainSetStatusCallCount);
    NDS_DIAG_RESET0(gNdsFighterWalkAnimEventsCallCount);
    NDS_DIAG_RESET0(gNdsFighterWalkCallbackReadyCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopInterruptCallCount);
    NDS_DIAG_RESET0(gNdsFighterWalkDeferredInterruptCheckCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterWalkP0GroundVelBeforeMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkP1GroundVelBeforeMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkP0GroundVelAfterMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkP1GroundVelAfterMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkP0AirVelXMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkP1AirVelXMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkP0AirVelYMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkP1AirVelYMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterWalkGroundVelAbsStickCount);
    NDS_DIAG_RESET0(gNdsFighterWalkGroundVelTransferAirCount);
    NDS_DIAG_RESET0(gNdsFighterWalkPhysicsCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterWalkMapCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterWalkMapSafeFloorCount);
    NDS_DIAG_RESET0(gNdsFighterWalkMapFallDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterWalkMapOttottoDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterWalkP0RootXBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWalkP0RootXAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWalkP0RootYBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWalkP0RootYAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWalkP1RootXBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWalkP1RootXAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWalkP1RootYBeforeBits);
    NDS_DIAG_RESET0(gNdsFighterWalkP1RootYAfterBits);
    NDS_DIAG_RESET0(gNdsFighterWalkGObjDelta);
    NDS_DIAG_RESET0(gNdsFighterWalkDeniedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterWalkUnexpectedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterWalkProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterWalkDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterWalkGameplayUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterWalkDrawCallCount);
    NDS_DIAG_RESET0(gNdsFighterWalkMatrixCallCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWalkLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWalkLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWalkLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWalkLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxWalkLoopCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopFrameTarget);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0HeldFrameCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1HeldFrameCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0InterruptCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1InterruptCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0PhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1PhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0IntegrateCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1IntegrateCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0MapCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1MapCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0SafeFloorCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1SafeFloorCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0StickX);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1StickX);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0StickAbs);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1StickAbs);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0LR);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1LR);
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
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0RootXStartBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0RootXAfterHeldBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0RootXAfterSettleBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1RootXStartBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1RootXAfterHeldBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1RootXAfterSettleBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0RootYStartBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0RootYAfterHeldBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0RootYAfterSettleBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1RootYStartBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1RootYAfterHeldBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1RootYAfterSettleBits);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0RootDeltaXMilli);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1RootDeltaXMilli);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0HeldRootDeltaXMilli);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1HeldRootDeltaXMilli);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP0RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopP1RootDirectionOK);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterWalkLoopP0GroundVelStartMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkLoopP1GroundVelStartMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkLoopP0GroundVelAfterHeldMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkLoopP1GroundVelAfterHeldMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkLoopP0GroundVelAfterSettleMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkLoopP1GroundVelAfterSettleMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkLoopP0AirVelXAfterHeldMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkLoopP1AirVelXAfterHeldMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkLoopP0AirVelYAfterHeldMilli)
            NDS_DIAG_WORD0(gNdsFighterWalkLoopP1AirVelYAfterHeldMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterWalkLoopGroundVelAbsStickCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopGroundVelTransferAirCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopWaitReturnCheckCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopWaitReturnSuccessCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopWaitSetStatusCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopWaitFrictionCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopReleaseInputCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopMapSafeFloorCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopMapFallDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopMapOttottoDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopGObjDelta);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopUnexpectedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopDeniedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopGameplayUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopDrawCallCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopMatrixCallCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopRootYDriftCount);
    NDS_DIAG_RESET0(gNdsFighterWalkLoopGADriftCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDashRunResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDashRunSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDashRunMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDashRunDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxDashRunCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDashRunWaitInterruptCallCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGroundCheckCallCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunOriginalDashCheckCallCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunOriginalDashCheckSuccessCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack1CheckCallCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack1CheckSuccessCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack100StartCheckCallCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackDashCheckCallCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackDashCheckSuccessCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunDashSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunRunSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunRunBrakeSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack11SetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack12SetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack13SetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack100StartSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack100LoopSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackDashSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunDashInterruptCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunRunInterruptCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunRunBrakeInterruptCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunDashPhysicsCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunRunPhysicsCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunRunBrakePhysicsCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunDashMapCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunRunMapCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunRunBrakeMapCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunSafeFloorCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFallBreakSafeCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunDeferredInterruptCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainDashStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainRunStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainRunBrakeStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainAttack11StatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainAttack12StatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainAttack13StatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainAttack100StartStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainAttack100LoopStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainAttackDashStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunAnimEventsCallCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGroundVelFrictionCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGroundVelTransferAirCount)
            NDS_DIAG_WORD(gNdsFighterDashRunP0StatusDash, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusDash, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0MotionDash, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionDash, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0StatusRun, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusRun, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0MotionRun, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionRun, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0StatusRunBrake, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusRunBrake, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0MotionRunBrake, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionRunBrake, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0StatusAttack11, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusAttack11, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0MotionAttack11, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionAttack11, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0StatusAttack12, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusAttack12, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0MotionAttack12, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionAttack12, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0StatusAttack13, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusAttack13, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0MotionAttack13, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionAttack13, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0StatusAttack100Start, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusAttack100Start, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0MotionAttack100Start, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionAttack100Start, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusAttack100Loop, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionAttack100Loop, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0StatusAttackDash, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusAttackDash, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0MotionAttackDash, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionAttackDash, 1u),
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack11CallbackMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack11TickMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack11WaitProcMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack12CallbackMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack12GotoMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack13CallbackMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack13GotoMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack100StartCallbackMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack100StartGotoMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack100LoopCallbackMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack100LoopGotoMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttack100LoopTickMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackAnimEventsMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventScriptMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventNoHitMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventCommandMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventParseCount)
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastPlayer, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastStatus, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastState, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastAttackID, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastGroupID, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastJointID, 1u),
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastDamage)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastSize)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastOffsetX)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastOffsetY)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastOffsetZ)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastAngle)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastKBG)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastKBW)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastBKB)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastShield)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventLastFlags)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventPositionMask)
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionState, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionAttackID, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionJointID, 1u),
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventPositionX)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventPositionY)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventPositionZ)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventPositionMatrixFlag)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackEventPositionMatrixValue)
            NDS_DIAG_WORD0(gNdsFighterDashRunDamageStatusMask)
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusLevel, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusIndex, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusGround, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusAir, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusElectric, 1u),
            NDS_DIAG_WORD0(gNdsFighterDashRunDamageSetupMask)
            NDS_DIAG_WORD(gNdsFighterDashRunDamageSetupStatusBefore, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageSetupStatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageSetupMotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageSetupGAAfter, 1u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterDashRunDamageSetupHitstunBefore = -1;
    gNdsFighterDashRunDamageSetupHitstunAfter = -1;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDashRunDamageSetupVelGroundMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunDamageSetupVelAirXMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunDamageSetupVelAirYMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunDamageSetupVelPhysicsMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardCheckCallCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardCheckSuccessCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainGuardOnStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardSetOffSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainGuardSetOffStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardAnimEventsMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardEffectCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardFGMCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardLastFGM)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0StatusGuardOn)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1StatusGuardOn)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0MotionGuardOn)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1MotionGuardOn)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardCallbackMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardStateMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardSetOffMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardSetOffCallbackMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardSetOffFramesMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunGuardSetOffVelMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunEscapeCheckCallCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunEscapeCheckSuccessCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunEscapeSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunFtMainEscapeStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunEscapeCallbackMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunEscapeStateMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunEscapeTickMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunEscapeInterruptCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunEscapePhysicsCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunEscapeMapCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0StatusEscape)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1StatusEscape)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0MotionEscape)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1MotionEscape)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0EscapeItemThrowBuffer)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1EscapeItemThrowBuffer)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackDashCallbackMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackDashTickMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunAttackDashRunProcMask)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0TapStickXAfterDash)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1TapStickXAfterDash)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0LR)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1LR)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0StickX)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1StickX)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0GroundVelRunMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1GroundVelRunMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0GroundVelBrakeMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1GroundVelBrakeMilli)
            NDS_DIAG_WORD0(gNdsFighterDashRunP0RootDirectionOK)
            NDS_DIAG_WORD0(gNdsFighterDashRunP1RootDirectionOK)
            NDS_DIAG_WORD0(gNdsFighterDashRunRootYDriftCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGADriftCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGObjDelta)
            NDS_DIAG_WORD0(gNdsFighterDashRunDeniedStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunUnexpectedStatusCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunProcessAttachCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunDisplayProbeCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunGameplayUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunDrawCallCount)
            NDS_DIAG_WORD0(gNdsFighterDashRunMatrixCallCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxJumpLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxJumpLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxJumpLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxJumpLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxJumpLoopCount);
    NDS_DIAG_RESET0(gNdsFighterJumpRunBrakeEndCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpWaitSetStatusCount);
    NDS_DIAG_RESET0(gNdsFighterJumpWaitInterruptCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpGroundCheckCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpOriginalKneeBendCheckCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpOriginalKneeBendCheckSuccessCount);
    NDS_DIAG_RESET0(gNdsFighterJumpKneeBendSetStatusCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpFtMainKneeBendStatusCount);
    NDS_DIAG_RESET0(gNdsFighterJumpKneeBendUpdateCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpKneeBendInterruptCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpSetStatusCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpFtMainJumpStatusCount);
    NDS_DIAG_RESET0(gNdsFighterJumpSetAirCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpAirInterruptCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpAirPhysicsCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpAirMapCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpGravityCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpAirDriftCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpAirFrictionCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpDeferredInterruptCheckCount);
    NDS_DIAG_RESET0(gNdsFighterJumpSpecialHiCheckCount);
    NDS_DIAG_RESET0(gNdsFighterJumpAttackHi4KneeBendCheckCount);
    NDS_DIAG_RESET0(gNdsFighterJumpSpecialAirCheckCount);
    NDS_DIAG_RESET0(gNdsFighterJumpAttackAirCheckCount);
    NDS_DIAG_RESET0(gNdsFighterJumpAttackAirRefreshCount);
    NDS_DIAG_RESET0(gNdsFighterJumpAttackAirRefreshMask);
    NDS_DIAG_RESET0(gNdsFighterJumpAttackAirRefreshStateMask);
    NDS_DIAG_RESET0(gNdsFighterJumpAttackAirRecordClearMask);
    NDS_DIAG_RESET0(gNdsFighterJumpAttackAirMapLandingMask);
    NDS_DIAG_RESET0(gNdsFighterJumpAttackAirDirectionMask);
    NDS_DIAG_RESET0(gNdsFighterJumpAerialCheckCount);
    NDS_DIAG_RESET0(gNdsFighterJumpHammerHoldCheckCount);
    NDS_DIAG_RESET0(gNdsFighterJumpHammerKneeBendCheckCount);
    NDS_DIAG_RESET0(gNdsFighterJumpFallDeferredCount);
    NDS_DIAG_RESET0(gNdsFighterJumpLandingDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterJumpCliffDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterJumpCeilingDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterJumpDeniedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterJumpUnexpectedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterJumpProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterJumpDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterJumpGameplayUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterJumpDrawCallCount);
    NDS_DIAG_RESET0(gNdsFighterJumpMatrixCallCount);
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
    NDS_DIAG_RESET0(gNdsFighterJumpP0InputSource);
    NDS_DIAG_RESET0(gNdsFighterJumpP1InputSource);
    NDS_DIAG_RESET0(gNdsFighterJumpP0ShortHop);
    NDS_DIAG_RESET0(gNdsFighterJumpP1ShortHop);
    NDS_DIAG_RESET0(gNdsFighterJumpP0StickX);
    NDS_DIAG_RESET0(gNdsFighterJumpP1StickX);
    NDS_DIAG_RESET0(gNdsFighterJumpP0ButtonTap);
    NDS_DIAG_RESET0(gNdsFighterJumpP1ButtonTap);
    NDS_DIAG_RESET0(gNdsFighterJumpP0ButtonRelease);
    NDS_DIAG_RESET0(gNdsFighterJumpP1ButtonRelease);
    NDS_DIAG_RESET0(gNdsFighterJumpP0KneeBendFrames);
    NDS_DIAG_RESET0(gNdsFighterJumpP1KneeBendFrames);
    NDS_DIAG_RESET0(gNdsFighterJumpP0AirFrames);
    NDS_DIAG_RESET0(gNdsFighterJumpP1AirFrames);
    NDS_DIAG_RESET0(gNdsFighterJumpP0RootDeltaXMilli);
    NDS_DIAG_RESET0(gNdsFighterJumpP1RootDeltaXMilli);
    NDS_DIAG_RESET0(gNdsFighterJumpP0RootDeltaYMilli);
    NDS_DIAG_RESET0(gNdsFighterJumpP1RootDeltaYMilli);
    NDS_DIAG_RESET0(gNdsFighterJumpP0RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterJumpP1RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterJumpP0RootRiseOK);
    NDS_DIAG_RESET0(gNdsFighterJumpP1RootRiseOK);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterJumpP0VelXInitialMilli)
            NDS_DIAG_WORD0(gNdsFighterJumpP1VelXInitialMilli)
            NDS_DIAG_WORD0(gNdsFighterJumpP0VelYInitialMilli)
            NDS_DIAG_WORD0(gNdsFighterJumpP1VelYInitialMilli)
            NDS_DIAG_WORD0(gNdsFighterJumpP0VelXAfterMilli)
            NDS_DIAG_WORD0(gNdsFighterJumpP1VelXAfterMilli)
            NDS_DIAG_WORD0(gNdsFighterJumpP0VelYAfterMilli)
            NDS_DIAG_WORD0(gNdsFighterJumpP1VelYAfterMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterJumpGObjDelta);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxLandingLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxLandingLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxLandingLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxLandingLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxLandingLoopCount);
    NDS_DIAG_RESET0(gNdsFighterLandingJumpAnimEndCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingFallSetStatusCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingFtMainFallStatusCount);
    NDS_DIAG_RESET0(gNdsFighterLandingSetGroundCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingSetStatusCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingFtMainLandingLightStatusCount);
    NDS_DIAG_RESET0(gNdsFighterLandingFtMainLandingHeavyStatusCount);
    NDS_DIAG_RESET0(gNdsFighterLandingEndCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingWaitSetStatusCount);
    NDS_DIAG_RESET0(gNdsFighterLandingWaitSetStatusSuccessCount);
    NDS_DIAG_RESET0(gNdsFighterLandingFallFrameMax);
    NDS_DIAG_RESET0(gNdsFighterLandingLandingFrameTarget);
    NDS_DIAG_RESET0(gNdsFighterLandingP0FallFrameCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP1FallFrameCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP0FallInterruptCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP1FallInterruptCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP0FallPhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP1FallPhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP0FallMapCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP1FallMapCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP0LandingFrameCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP1LandingFrameCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP0LandingInterruptCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP1LandingInterruptCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP0LandingPhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterLandingP1LandingPhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterLandingAirNoCollisionCount);
    NDS_DIAG_RESET0(gNdsFighterLandingFloorDetectCount);
    NDS_DIAG_RESET0(gNdsFighterLandingFloorClampCount);
    NDS_DIAG_RESET0(gNdsFighterLandingFastFallCheckCount);
    NDS_DIAG_RESET0(gNdsFighterLandingFastFallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingHeavyDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterLandingFallAerialDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterLandingJumpAerialDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterLandingCliffDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterLandingCeilingDeniedCount);
    NDS_DIAG_RESET0(gNdsFighterLandingDeferredInterruptCheckCount);
    NDS_DIAG_RESET0(gNdsFighterLandingGObjDelta);
    NDS_DIAG_RESET0(gNdsFighterLandingUnexpectedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterLandingDeniedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterLandingProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterLandingDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterLandingGameplayUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterLandingDrawCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingMatrixCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingRootYDriftCount);
    NDS_DIAG_RESET0(gNdsFighterLandingGADriftCount);
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterLandingP0FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP1FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP0RootYFallStartMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP1RootYFallStartMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP0RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP1RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP0RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP1RootDeltaXMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterLandingP0RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterLandingP1RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterLandingP0RootFloorOK);
    NDS_DIAG_RESET0(gNdsFighterLandingP1RootFloorOK);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterLandingP0VelYFallStartMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP1VelYFallStartMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP0VelYBeforeLandingMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP1VelYBeforeLandingMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP0GroundVelAfterLandingMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP1GroundVelAfterLandingMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP0GroundVelAfterWaitMilli)
            NDS_DIAG_WORD0(gNdsFighterLandingP1GroundVelAfterWaitMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterLandingGravityCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingAirDriftCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingAirFrictionCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingGroundFrictionCallCount);
    NDS_DIAG_RESET0(gNdsFighterLandingWaitFrictionCallCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxProcessLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxProcessLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxProcessLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxProcessLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxProcessLoopCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopFrameMax);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0FrameCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1FrameCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0Completed);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1Completed);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0StatusVisitMask);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1StatusVisitMask);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0TransitionMask);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1TransitionMask);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0InputApplyCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1InputApplyCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopControllerBridgeCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopControllerMirrorCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0ButtonTapMask);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1ButtonTapMask);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0LastStickX);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1LastStickX);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0UpdateCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1UpdateCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0InterruptCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1InterruptCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0PhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1PhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0IntegrateCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1IntegrateCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0MapCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1MapCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0WaitVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1WaitVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0WalkVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1WalkVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0DashVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1DashVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0RunVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1RunVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0RunBrakeVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1RunBrakeVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0KneeBendVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1KneeBendVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0JumpVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1JumpVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0FallVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1FallVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0LandingVisitCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1LandingVisitCount);
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP0FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP1FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP0RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP1RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP0RootXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP1RootXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP0RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP1RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP0RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP1RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP0RootRiseMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP1RootRiseMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP0FloorOK);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopP1FloorOK);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP0GroundVelFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP1GroundVelFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP0AirVelXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP1AirVelXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP0AirVelYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterProcessLoopP1AirVelYFinalMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterProcessLoopFallDetectCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopLandingDetectCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopSetGroundCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopSetAirCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopWaitSetStatusCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopRunBrakeEndCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopJumpAnimEndCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopLandingEndCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopDeferredInterruptCheckCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopGObjDelta);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopUnexpectedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopDeniedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopGameplayUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopDrawCallCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopMatrixCallCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopRootYDriftCount);
    NDS_DIAG_RESET0(gNdsFighterProcessLoopGADriftCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxSchedulerLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxSchedulerLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxSchedulerLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxSchedulerLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxSchedulerLoopCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopPrepared);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopFrameMax);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopUpdateMax);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopTaskmanUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopVSBattleUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopBaseVSBattleUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopSchedulerUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopGObjCountBefore);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopGObjCountAfter);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopGObjDelta);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0ProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1ProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopProcessAttachEscapeCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0GObjProcessRunCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1GObjProcessRunCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0ProcCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1ProcCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0InputApplyCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1InputApplyCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopControllerBridgeCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopControllerMirrorCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0ButtonTapMask);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1ButtonTapMask);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0LastStickX);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1LastStickX);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0FrameCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1FrameCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0Completed);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1Completed);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0StatusVisitMask);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1StatusVisitMask);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0TransitionMask);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1TransitionMask);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0WaitVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1WaitVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0WalkVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1WalkVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0DashVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1DashVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0RunVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1RunVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0RunBrakeVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1RunBrakeVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0KneeBendVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1KneeBendVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0JumpVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1JumpVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0FallVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1FallVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0LandingVisitCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1LandingVisitCount);
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP0FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP1FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP0RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP1RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP0RootXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP1RootXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP0RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP1RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP0RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP1RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP0RootRiseMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP1RootRiseMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0FloorOK);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1FloorOK);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP0GroundVelFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP1GroundVelFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP0AirVelXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP1AirVelXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP0AirVelYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterSchedulerLoopP1AirVelYFinalMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0UpdateCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1UpdateCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0InterruptCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1InterruptCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0PhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1PhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0IntegrateCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1IntegrateCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP0MapCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopP1MapCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopFallDetectCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopLandingDetectCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopSetGroundCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopSetAirCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopWaitSetStatusCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopRunBrakeEndCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopJumpAnimEndCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopLandingEndCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopDeferredInterruptCheckCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopUnexpectedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopDeniedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopGameplayUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopDrawCallCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopMatrixCallCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopRootYDriftCount);
    NDS_DIAG_RESET0(gNdsFighterSchedulerLoopGADriftCount);
    ndsControllerPlaybackReset();
    NDS_DIAG_RESET0(gNdsControllerPollCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxControllerLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxControllerLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxControllerLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxControllerLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxControllerLoopCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterControllerLoopPrepared)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopFrameMax)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopUpdateMax)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopTaskmanUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopVSBattleUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopBaseVSBattleUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopSchedulerUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopSYReadCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopSYUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopGObjCountBefore)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopGObjCountAfter)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopGObjDelta)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0ProcessAttachCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1ProcessAttachCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopProcessAttachEscapeCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0GObjProcessRunCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1GObjProcessRunCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0ProcCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1ProcCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0PlaybackApplyCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1PlaybackApplyCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0ControllerToFTInputCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1ControllerToFTInputCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0DirectFTInputWriteCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1DirectFTInputWriteCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0ButtonTapMask)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1ButtonTapMask)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0ButtonHoldMask)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1ButtonHoldMask)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0ButtonReleaseMask)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1ButtonReleaseMask)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0LastStickX)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1LastStickX)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0LastStickY)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1LastStickY)
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0TapStickXMin, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1TapStickXMin, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0TapStickYMin, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1TapStickYMin, 1u),
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0DashTapEligibleCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1DashTapEligibleCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0JumpButtonTapCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1JumpButtonTapCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0FrameCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1FrameCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0Completed)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1Completed)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0StatusVisitMask)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1StatusVisitMask)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0TransitionMask)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1TransitionMask)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0WaitVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1WaitVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0WalkVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1WalkVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0DashVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1DashVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0RunVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1RunVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0RunBrakeVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1RunBrakeVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0KneeBendVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1KneeBendVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0JumpVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1JumpVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0FallVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1FallVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0LandingVisitCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1LandingVisitCount)
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0StatusStart, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1StatusStart, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0MotionStart, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1MotionStart, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0StatusFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1StatusFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0MotionFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1MotionFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0GAFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1GAFinal, 1u),
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0RootXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1RootXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0RootRiseMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1RootRiseMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0RootDirectionOK)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1RootDirectionOK)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0FloorOK)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1FloorOK)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0GroundVelFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1GroundVelFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0AirVelXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1AirVelXFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0AirVelYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1AirVelYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0UpdateCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1UpdateCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0InterruptCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1InterruptCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0PhysicsCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1PhysicsCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0IntegrateCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1IntegrateCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP0MapCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopP1MapCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopFallDetectCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopLandingDetectCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopSetGroundCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopSetAirCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopWaitSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopRunBrakeEndCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopJumpAnimEndCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopLandingEndCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopDeferredInterruptCheckCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopUnexpectedStatusCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopDeniedStatusCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopDisplayProbeCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopGameplayUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopDrawCallCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopMatrixCallCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopRootYDriftCount)
            NDS_DIAG_WORD0(gNdsFighterControllerLoopGADriftCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxPreviewLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxPreviewLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxPreviewLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxPreviewLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxPreviewLoopCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopPrepared);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopFrameMax);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopUpdateMax);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopTaskmanUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopVSBattleUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopBaseVSBattleUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopSchedulerUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopSYReadCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopSYUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopGObjCountBefore);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopGObjCountAfter);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopGObjDelta);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0ProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1ProcessAttachCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopProcessAttachEscapeCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0GObjProcessRunCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1GObjProcessRunCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0ProcCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1ProcCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0PlaybackApplyCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1PlaybackApplyCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0ControllerToFTInputCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1ControllerToFTInputCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0DirectFTInputWriteCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1DirectFTInputWriteCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0ButtonTapMask);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1ButtonTapMask);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0ButtonHoldMask);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1ButtonHoldMask);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0LastStickX);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1LastStickX);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0LastStickY);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1LastStickY);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0DashTapEligibleCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1DashTapEligibleCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0JumpButtonTapCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1JumpButtonTapCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0FrameCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1FrameCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0Completed);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1Completed);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0StatusVisitMask);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1StatusVisitMask);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0TransitionMask);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1TransitionMask);
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP0RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP1RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP0RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP1RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP0RootRiseMilli)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP1RootRiseMilli)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP0RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP1RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP0FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP1FloorYMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1RootDirectionOK);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0FloorOK);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1FloorOK);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0InterruptCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1InterruptCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0PhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1PhysicsCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0IntegrateCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1IntegrateCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0MapCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1MapCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopPreviewWidth);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopPreviewHeight);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopPreviewPitch);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopPreviewReady);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopPreviewCommitBefore);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopPreviewCommitAfter);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopPreviewCommitDelta);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopDrawFrameCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopDisplayCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0DisplayCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1DisplayCallbackCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0CandidateCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1CandidateCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0DrawnDObjCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1DrawnDObjCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0PixelCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1PixelCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopTotalPixelCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0ColorChecksum);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1ColorChecksum);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP0ScreenXStart)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP1ScreenXStart)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP0ScreenXFinal)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP1ScreenXFinal)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP0ScreenXDelta)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP1ScreenXDelta)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP0ScreenYFloor)
            NDS_DIAG_WORD0(gNdsFighterPreviewLoopP1ScreenYFloor)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterPreviewLoopP0ScreenYMin = 0x7fffffff;
    gNdsFighterPreviewLoopP1ScreenYMin = 0x7fffffff;
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP0ScreenRise);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopP1ScreenRise);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopFallDetectCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopLandingDetectCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopSetGroundCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopSetAirCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopWaitSetStatusCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopRunBrakeEndCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopJumpAnimEndCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopLandingEndCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopDeferredInterruptCheckCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopUnexpectedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopDeniedStatusCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopDisplayProbeCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopGameplayUpdateCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopDrawCallCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopMatrixCallCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopRootYDriftCount);
    NDS_DIAG_RESET0(gNdsFighterPreviewLoopGADriftCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGCRunAllLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGCRunAllLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGCRunAllLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGCRunAllLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGCRunAllLoopCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopPrepared)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopFrameMax)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopUpdateMax)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopTaskmanUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopVSBattleUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopBaseVSBattleUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopRunAllCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopSYReadCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopSYUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopGObjCountBefore)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopGObjCountAfter)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopGObjDelta)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopOldProcessPauseCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopNonTargetGObjVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopNonTargetProcessPauseCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopTargetProcessPreserveCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ProcessAttachCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ProcessAttachCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopProcessAttachEscapeCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0GObjProcessRunCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1GObjProcessRunCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ProcCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ProcCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0PlaybackApplyCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1PlaybackApplyCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ControllerToFTInputCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ControllerToFTInputCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0DirectFTInputWriteCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1DirectFTInputWriteCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ButtonTapMask)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ButtonTapMask)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ButtonHoldMask)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ButtonHoldMask)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0LastStickX)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1LastStickX)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0LastStickY)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1LastStickY)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0DashTapEligibleCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1DashTapEligibleCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0JumpButtonTapCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1JumpButtonTapCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0FrameCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1FrameCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0Completed)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1Completed)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0StatusVisitMask)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1StatusVisitMask)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0TransitionMask)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1TransitionMask)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0WaitVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1WaitVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0WalkVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1WalkVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0DashVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1DashVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0RunVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1RunVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0RunBrakeVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1RunBrakeVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0KneeBendVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1KneeBendVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0JumpVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1JumpVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0FallVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1FallVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0LandingVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1LandingVisitCount)
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0StatusStart, 1u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1StatusStart, 1u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0MotionStart, 1u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1MotionStart, 1u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0StatusFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1StatusFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0MotionFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1MotionFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0GAFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1GAFinal, 1u),
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0RootRiseMilli)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1RootRiseMilli)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0RootDirectionOK)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1RootDirectionOK)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0FloorOK)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1FloorOK)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0InterruptCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1InterruptCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0PhysicsCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1PhysicsCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0IntegrateCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1IntegrateCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0MapCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1MapCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopPreviewWidth)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopPreviewHeight)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopPreviewPitch)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopPreviewReady)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopPreviewCommitBefore)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopPreviewCommitAfter)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopPreviewCommitDelta)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopDrawFrameCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopDisplayCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0DisplayCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1DisplayCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0CandidateCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1CandidateCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0DrawnDObjCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1DrawnDObjCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0PixelCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1PixelCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopTotalPixelCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ColorChecksum)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ColorChecksum)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ScreenXStart)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ScreenXStart)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ScreenXFinal)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ScreenXFinal)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ScreenXDelta)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ScreenXDelta)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ScreenYFloor)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ScreenYFloor)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterGCRunAllLoopP0ScreenYMin = 0x7fffffff;
    gNdsFighterGCRunAllLoopP1ScreenYMin = 0x7fffffff;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP0ScreenRise)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopP1ScreenRise)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopFallDetectCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopLandingDetectCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopSetGroundCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopSetAirCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopWaitSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopRunBrakeEndCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopJumpAnimEndCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopLandingEndCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopDeferredInterruptCheckCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopUnexpectedStatusCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopDeniedStatusCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopDisplayProbeCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopGameplayUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopDrawCallCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopMatrixCallCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopRootYDriftCount)
            NDS_DIAG_WORD0(gNdsFighterGCRunAllLoopGADriftCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGCDrawAllLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGCDrawAllLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGCDrawAllLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGCDrawAllLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxGCDrawAllLoopCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopPrepared)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopFrameMax)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopUpdateMax)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopTaskmanUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopVSBattleUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopBaseVSBattleUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopDrawAllCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopCameraCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopCapturedDisplayCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopNonTargetDisplayCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopRunAllCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopSYReadCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopSYUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopGObjCountBefore)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopGObjCountAfter)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopGObjDelta)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopOldProcessPauseCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopNonTargetGObjVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopNonTargetProcessPauseCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopTargetProcessPreserveCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ProcessAttachCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ProcessAttachCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopProcessAttachEscapeCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0GObjProcessRunCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1GObjProcessRunCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ProcCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ProcCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0PlaybackApplyCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1PlaybackApplyCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ControllerToFTInputCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ControllerToFTInputCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0DirectFTInputWriteCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1DirectFTInputWriteCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ButtonTapMask)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ButtonTapMask)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ButtonHoldMask)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ButtonHoldMask)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0LastStickX)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1LastStickX)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0LastStickY)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1LastStickY)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0DashTapEligibleCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1DashTapEligibleCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0JumpButtonTapCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1JumpButtonTapCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0FrameCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1FrameCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0Completed)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1Completed)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0StatusVisitMask)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1StatusVisitMask)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0TransitionMask)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1TransitionMask)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0WaitVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1WaitVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0WalkVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1WalkVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0DashVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1DashVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0RunVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1RunVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0RunBrakeVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1RunBrakeVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0KneeBendVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1KneeBendVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0JumpVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1JumpVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0FallVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1FallVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0LandingVisitCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1LandingVisitCount)
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0StatusStart, 1u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1StatusStart, 1u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0MotionStart, 1u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1MotionStart, 1u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0StatusFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1StatusFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0MotionFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1MotionFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0GAFinal, 1u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1GAFinal, 1u),
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1RootXStartMilli)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1RootDeltaXMilli)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0RootRiseMilli)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1RootRiseMilli)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1FloorYMilli)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0RootDirectionOK)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1RootDirectionOK)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0FloorOK)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1FloorOK)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0InterruptCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1InterruptCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0PhysicsCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1PhysicsCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0IntegrateCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1IntegrateCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0MapCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1MapCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopPreviewWidth)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopPreviewHeight)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopPreviewPitch)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopPreviewReady)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopPreviewCommitBefore)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopPreviewCommitAfter)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopPreviewCommitDelta)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopDrawFrameCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopDisplayCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0DisplayCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1DisplayCallbackCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0CandidateCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1CandidateCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0DrawnDObjCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1DrawnDObjCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0PixelCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1PixelCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopTotalPixelCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ColorChecksum)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ColorChecksum)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ScreenXStart)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ScreenXStart)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ScreenXFinal)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ScreenXFinal)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ScreenXDelta)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ScreenXDelta)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ScreenYFloor)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ScreenYFloor)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterGCDrawAllLoopP0ScreenYMin = 0x7fffffff;
    gNdsFighterGCDrawAllLoopP1ScreenYMin = 0x7fffffff;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP0ScreenRise)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopP1ScreenRise)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopFallDetectCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopLandingDetectCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopSetGroundCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopSetAirCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopWaitSetStatusCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopRunBrakeEndCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopJumpAnimEndCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopLandingEndCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopDeferredInterruptCheckCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopUnexpectedStatusCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopDeniedStatusCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopDisplayProbeCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopGameplayUpdateCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopDrawCallCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopMatrixCallCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopRootYDriftCount)
            NDS_DIAG_WORD0(gNdsFighterGCDrawAllLoopGADriftCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageGCDrawAllLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageGCDrawAllLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageGCDrawAllLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageGCDrawAllLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageGCDrawAllLoopCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopPrepared);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopBaseResultSeen);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopDrawAllCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopCameraCallbackCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopCapturedDisplayCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopLayerCaptureMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopMapCaptureMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopDObjDrawCallbackCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopDObjDrawKindMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopLayerDObjMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopMapDObjMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopLayerDLReadyMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopMapDLReadyMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopLayerMObjMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopMapMObjMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopNonStageCaptureCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopFighterDisplayCallbackCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopUnexpectedSceneCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopManualDisplayCallCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopGObjCountBefore);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopGObjCountAfter);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopGObjCountDelta);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopPreviewCommitDelta);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopTotalPixelCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopCompatMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareSubmitCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareTriangleCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareTextureBindCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareTextureUploadCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareTextureReadyCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareTextureRejectCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareTextureFormatMask);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareTextureMaxWidth);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareTextureMaxHeight);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareFighterSubmitCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareFighterTriangleCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsFighterDisplayContractSelectedCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractHiddenCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractNoTextureCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractSubmittedCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractGeometryMode)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractCycleType)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractRenderMode)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractLightCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractLightDirectionCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractBoundsPassCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractBoundsFailCount)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractBoundsXBits)
            NDS_DIAG_WORD0(gNdsFighterDisplayContractBoundsYBits)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareCarrySeedCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareCarryCaptureCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount);
    NDS_DIAG_RESET0(gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageCollisionLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageCollisionLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageCollisionLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageCollisionLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageCollisionLoopCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopPrepared);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopBaseStageDrawSeen);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopGeometryReady);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopGroundDataReady);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopYakumonoCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopMapObjCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopFloorLineCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopTotalLineCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopProjectCallCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopGeometryProjectCallCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopLegacyFlatFallbackCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopNoGeometryCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopOutOfRangeLineCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopBadVertexCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopDivisionGuardCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopProbeCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopProbeHitCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopProbeMissCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopOffstageMissCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopBelowFloorMissCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP0ProjectCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP1ProjectCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP0HitCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP1HitCount);
    gNdsStageCollisionLoopP0FloorLineID = -1;
    gNdsStageCollisionLoopP1FloorLineID = -1;
    gNdsStageCollisionLoopP0FloorKind = 0xffffffffu;
    gNdsStageCollisionLoopP1FloorKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP0FloorLineIsFloor);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP1FloorLineIsFloor);
    gNdsStageCollisionLoopFloorGroupID = -1;
    NDS_DIAG_RESET0(gNdsStageCollisionLoopFloorGroupCount);
    gNdsStageCollisionLoopFloorLineMin = -1;
    gNdsStageCollisionLoopFloorLineMaxExclusive = -1;
    NDS_DIAG_RESET0(gNdsStageCollisionLoopNonFloorCandidateCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopYakumonoDObjDeferredCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopYakumonoDObjUnsafeIndexGuardCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP0FloorDistMilli);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP1FloorDistMilli);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP0FloorFlags);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP1FloorFlags);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP0FloorAngleX1000)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP0FloorAngleY1000)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP1FloorAngleX1000)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP1FloorAngleY1000)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP0EdgeLX)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP0EdgeLY)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP0EdgeRX)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP0EdgeRY)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP1EdgeLX)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP1EdgeLY)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP1EdgeRX)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP1EdgeRY)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP0RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP1RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP0FloorYMilli)
            NDS_DIAG_WORD0(gNdsStageCollisionLoopP1FloorYMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP0FloorOK);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopP1FloorOK);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopGObjDelta);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopUnexpectedSceneCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopUnexpectedStatusCount);
    NDS_DIAG_RESET0(gNdsStageCollisionLoopUnsafeFallbackAfterPrepareCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageFloorFollowLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageFloorFollowLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageFloorFollowLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageFloorFollowLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageFloorFollowLoopCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopPrepared);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopBaseDrawSeen);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopBaseCollisionSeen);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopGeometryReady);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopInitialSeedCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopInitialAdoptCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopFinalRecenterCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopFinalAdoptCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopMapUpdateCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopP0MapUpdateCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopP1MapUpdateCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopProjectCallCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopGeometryHitCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopGeometryMissCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopNoGeometryCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopNonFloorLineCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopClampCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopNoClampCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopP0HitCount);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopP1HitCount);
    gNdsStageFloorFollowLoopP0FloorLineID = -1;
    gNdsStageFloorFollowLoopP1FloorLineID = -1;
    gNdsStageFloorFollowLoopP0FloorKind = 0xffffffffu;
    gNdsStageFloorFollowLoopP1FloorKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopP0FloorLineIsFloor);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopP1FloorLineIsFloor);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP0InitialRootXMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP1InitialRootXMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP0FinalRootXMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP1FinalRootXMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP0RootXDeltaMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP1RootXDeltaMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP0FinalRootYMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP1FinalRootYMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP0FloorYMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP1FloorYMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP0FinalDriftMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP1FinalDriftMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP0MaxDriftMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopP1MaxDriftMilli)
            NDS_DIAG_WORD0(gNdsStageFloorFollowLoopMaxDriftMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopP0FloorOK);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopP1FloorOK);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopP0FloorVisitMask);
    NDS_DIAG_RESET0(gNdsStageFloorFollowLoopP1FloorVisitMask);
    gNdsStageFloorFollowLoopP0StatusFinal = 0xffffffffu;
    gNdsStageFloorFollowLoopP1StatusFinal = 0xffffffffu;
    gNdsStageFloorFollowLoopP0GAFinal = 0xffffffffu;
    gNdsStageFloorFollowLoopP1GAFinal = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageFloorEdgeLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageFloorEdgeLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageFloorEdgeLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageFloorEdgeLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageFloorEdgeLoopCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopPrepared);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopGeometryReady);
    gNdsStageFloorEdgeLoopSelectedLineID = -1;
    gNdsStageFloorEdgeLoopSelectedLineKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopSelectedVertexCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStageFloorEdgeLoopLeftXMilli)
            NDS_DIAG_WORD0(gNdsStageFloorEdgeLoopRightXMilli)
            NDS_DIAG_WORD0(gNdsStageFloorEdgeLoopWidthMilli)
            NDS_DIAG_WORD0(gNdsStageFloorEdgeLoopP0StartDistMilli)
            NDS_DIAG_WORD0(gNdsStageFloorEdgeLoopP1StartDistMilli)
            NDS_DIAG_WORD0(gNdsStageFloorEdgeLoopP0FinalDistMilli)
            NDS_DIAG_WORD0(gNdsStageFloorEdgeLoopP1FinalDistMilli)
            NDS_DIAG_WORD0(gNdsStageFloorEdgeLoopP0DeltaDistMilli)
            NDS_DIAG_WORD0(gNdsStageFloorEdgeLoopP1DeltaDistMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsStageFloorEdgeLoopP0MinDistMilli = 0x7fffffff;
    gNdsStageFloorEdgeLoopP1MinDistMilli = 0x7fffffff;
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP0ApproachOK);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP1ApproachOK);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP0NearEdgeOK);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP1NearEdgeOK);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP0FloorOK);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP1FloorOK);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP0FloorVisitMask);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP1FloorVisitMask);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopInsideProbeCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopInsideProbeHitCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopOutsideProbeCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopOutsideProbeMissCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopOutsideProbeUnexpectedHitCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopFCCommonCallCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopFCCommonHitCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopLineTypeCallCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopVertexPositionCallCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopEdgeUnderLCallCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopEdgeUnderRCallCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopEdgeUnderDeferredCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopMapUpdateCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP0MapUpdateCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP1MapUpdateCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopPreClampDriftSampleCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopPreClampCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP0MaxPreClampDriftMilli);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopP1MaxPreClampDriftMilli);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopMaxPreClampDriftMilli);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopFinalRecenterCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopFinalAdoptCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopUnexpectedSceneCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopUnexpectedStatusCount);
    NDS_DIAG_RESET0(gNdsStageFloorEdgeLoopUnsafeFallbackAfterPrepareCount);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPProcessFloorLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPProcessFloorLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPProcessFloorLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPProcessFloorLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPProcessFloorLoopCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopPrepared)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopBaseFloorEdgeSeen)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopAdapterBuildCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopAdapterCopyBackCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopAdapterFallbackLRCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopProjectFloorIDCallCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopProjectFloorIDHitCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopProjectFloorIDMissCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopTestNewCallCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopTestNewHitCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopTestNewMissCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopTestNewEdgeBranchCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopTestNewSetProjectCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopSetLandingFloorCallCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopSetCollideFloorCallCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopFCCommonPositiveDistCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopFCCommonNegativeDistCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopFCCommonZeroDistCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP0UpdateCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP1UpdateCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP0HitCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP1HitCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP0MissCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP1MissCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsStageMPProcessFloorLoopP0FinalLineID = -1;
    gNdsStageMPProcessFloorLoopP1FinalLineID = -1;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP0FinalLineIsFloor)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP1FinalLineIsFloor)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP0FinalMaskStat)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP1FinalMaskStat)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP0FinalDistMilli)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP1FinalDistMilli)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP0RootYMilli)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopP1RootYMilli)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopInsideProbeCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopInsideProbeHitCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopOutsideProbeCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopOutsideProbeMissCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopBelowFloorProbeCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopBelowFloorPositiveDistCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopNoFinalRecenterCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopUnexpectedSceneCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopUnexpectedStatusCount)
            NDS_DIAG_WORD0(gNdsStageMPProcessFloorLoopUnsafeCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPUpdateFloorLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPUpdateFloorLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPUpdateFloorLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPUpdateFloorLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPUpdateFloorLoopCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopPrepared)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopBaseMPProcessSeen)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAdapterBuildCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAdapterCopyBackCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAdapterFallbackLRCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopUpdateMainCallCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopUpdateMainReturnTrueCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopUpdateMainReturnFalseCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopUpdateMainStepCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopUpdateMainMaxStepCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopUpdateMainSplitCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopUpdateMainCapCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopTranslateResetCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopProcCollCallCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAllCollisionsCallCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAllCollisionsFloorHitCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAllCollisionsFloorMissCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAllCollisionsCliffEdgeBranchCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAllCollisionsStopEdgeBranchCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAllCollisionsDefaultEndCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAllCollisionsWallDeferredCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAllCollisionsCeilDeferredCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAllCollisionsFloorEdgeAdjustDeferredCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopCheckFloorCallCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopCheckCliffEdgeCallCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopCheckFloorHitCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopCheckCliffEdgeHitCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopCheckFloorMissCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopCheckCliffEdgeMissCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopInsideProbeCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopInsideProbeHitCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopOutsideProbeCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopOutsideProbeMissCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopBelowFloorProbeCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopBelowFloorHitCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopSplitProbeCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopSplitProbeStepCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0UpdateCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1UpdateCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0HitCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1HitCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0MissCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1MissCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0PosDiffXMilli)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1PosDiffXMilli)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0PosDiffYMilli)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1PosDiffYMilli)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0RootXBeforeMilli)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1RootXBeforeMilli)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0RootXFinalMilli)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1RootXFinalMilli)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0RootYFinalMilli)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1RootYFinalMilli)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsStageMPUpdateFloorLoopP0FinalLineID = -1;
    gNdsStageMPUpdateFloorLoopP1FinalLineID = -1;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0FinalLineIsFloor)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1FinalLineIsFloor)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0FinalMaskStat)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1FinalMaskStat)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP0FloorOK)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopP1FloorOK)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopNoFinalRecenterCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopFallDeniedCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopOttottoDeniedCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopUnexpectedSceneCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopUnexpectedStatusCount)
            NDS_DIAG_WORD0(gNdsStageMPUpdateFloorLoopUnsafeCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPSweepFloorLoopResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPSweepFloorLoopSafeResult);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPSweepFloorLoopMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPSweepFloorLoopDeferredMask);
    NDS_DIAG_RESET0(gNdsFighterMarioFoxStageMPSweepFloorLoopCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopPrepared)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopBaseMPUpdateSeen)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopCheckFloorCallCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopCheckFloorHitCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopCheckFloorMissCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLineSweepSameCallCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLineSweepSameHitCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLineSweepSameMissCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLineSweepDiffCallCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLineSweepDiffHitCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLineSweepDiffMissCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLineSweepVisitCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLineSweepCandidateCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLineSweepRejectSameLineCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLineSweepAcceptNewLineCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopSecondFloorCallCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopSecondFloorHitCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopSecondFloorMissCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopLandingFloorCallCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopFloorEdgeAdjustCallCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopFloorEdgeAdjustDeferredCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopMaskCurrFloorCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopMaskStatFloorEdgeClearCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopIsCollEndClearCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopSameLineProbeCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopSameLineProbeHitCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopDiffLineProbeCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopDiffLineProbeHitCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopNoHitProbeCount)
            NDS_DIAG_WORD0(gNdsStageMPSweepFloorLoopNoHitProbeMissCount)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsStageMPSweepFloorLoopProbeLineID = -1;
    gNdsStageMPSweepFloorLoopAltLineID = -1;
    gNdsStageMPSweepFloorLoopP0FinalLineID = -1;
    gNdsStageMPSweepFloorLoopP1FinalLineID = -1;
    NDS_DIAG_RESET0(gNdsStageMPSweepFloorLoopP0FinalLineIsFloor);
    NDS_DIAG_RESET0(gNdsStageMPSweepFloorLoopP1FinalLineIsFloor);
    NDS_DIAG_RESET0(gNdsStageMPSweepFloorLoopP0FloorOK);
    NDS_DIAG_RESET0(gNdsStageMPSweepFloorLoopP1FloorOK);
    NDS_DIAG_RESET0(gNdsStageMPSweepFloorLoopUnsafeCount);
    NDS_DIAG_RESET0(gNdsSCVSBattleStageResult);
    NDS_DIAG_RESET0(gNdsSCVSBattleStageMask);
    NDS_DIAG_RESET0(gNdsSCVSBattleStageGKind);
    NDS_DIAG_RESET0(gNdsSCVSBattleStageGroundDataReady);
    NDS_DIAG_RESET0(gNdsSCVSBattleStageGeometryReady);
    NDS_DIAG_RESET0(gNdsSCVSBattleStageMapNodesReady);
    NDS_DIAG_RESET0(gNdsSCVSBattleStageBGM);
    NDS_DIAG_RESET0(gNdsSCVSBattleStageLightAngleXBits);
    NDS_DIAG_RESET0(gNdsSCVSBattleStageLightAngleYBits);
    NDS_DIAG_RESET0(gNdsSCVSBattleStageDeferredMask);
    NDS_DIAG_RESET0(gNdsPupupuGroundSetupResult);
    NDS_DIAG_RESET0(gNdsPupupuGroundDisplayResult);
    NDS_DIAG_RESET0(gNdsPupupuGroundGObjResult);
    NDS_DIAG_RESET0(gNdsPupupuGroundSetupMask);
    NDS_DIAG_RESET0(gNdsPupupuGroundDeferredMask);
    NDS_DIAG_RESET0(gNdsPupupuGroundLayerGObjCount);
    NDS_DIAG_RESET0(gNdsPupupuGroundLayerGObjMask);
    NDS_DIAG_RESET0(gNdsPupupuGroundLayerDObjMask);
    NDS_DIAG_RESET0(gNdsPupupuGroundLayerMObjMask);
    NDS_DIAG_RESET0(gNdsPupupuGroundLayerAnimMask);
    NDS_DIAG_RESET0(gNdsPupupuGroundMapGObjCount);
    NDS_DIAG_RESET0(gNdsPupupuGroundMapGObjMask);
    NDS_DIAG_RESET0(gNdsPupupuGroundMapHeadReady);
    NDS_DIAG_RESET0(gNdsPupupuGroundMapHeadOffset);
    NDS_DIAG_RESET0(gNdsPupupuGroundRootGObjID);
    NDS_DIAG_RESET0(gNdsPupupuGroundWhispyEyesGObjID);
    NDS_DIAG_RESET0(gNdsPupupuGroundWhispyMouthGObjID);
    NDS_DIAG_RESET0(gNdsPupupuGroundFlowersBackGObjID);
    NDS_DIAG_RESET0(gNdsPupupuGroundFlowersFrontGObjID);
    NDS_DIAG_RESET0(gNdsPupupuGroundParticleBankID);
    NDS_DIAG_RESET0(gNdsPupupuGroundProcessAttachCount);
    NDS_DIAG_RESET0(gNdsPupupuGroundNonPupupuStubCallCount);
    NDS_DIAG_RESET0(gNdsPupupuGroundGObjCountBefore);
    NDS_DIAG_RESET0(gNdsPupupuGroundGObjCountAfter);
    NDS_DIAG_RESET0(gNdsPupupuGroundDObjCountAfter);
    NDS_DIAG_RESET0(gNdsPupupuGroundMObjCountAfter);
    NDS_DIAG_RESET0(gNdsPupupuGroundAObjCountAfter);
    NDS_DIAG_RESET0(gNdsPupupuUpdateResult);
    NDS_DIAG_RESET0(gNdsPupupuUpdateMask);
    NDS_DIAG_RESET0(gNdsPupupuUpdateTickCount);
    NDS_DIAG_RESET0(gNdsPupupuUpdateGameStatusBefore);
    NDS_DIAG_RESET0(gNdsPupupuUpdateGameStatusAfter);
    NDS_DIAG_RESET0(gNdsPupupuUpdateWhispyStatusBefore);
    NDS_DIAG_RESET0(gNdsPupupuUpdateWhispyStatusAfterFirst);
    NDS_DIAG_RESET0(gNdsPupupuUpdateWhispyStatusAfterFinal);
    NDS_DIAG_RESET0(gNdsPupupuUpdateWindWaitBefore);
    NDS_DIAG_RESET0(gNdsPupupuUpdateWindWaitAfterFirst);
    NDS_DIAG_RESET0(gNdsPupupuUpdateWindWaitAfterFinal);
    NDS_DIAG_RESET0(gNdsPupupuUpdateBlinkWaitBefore);
    NDS_DIAG_RESET0(gNdsPupupuUpdateBlinkWaitAfterFinal);
    NDS_DIAG_RESET0(gNdsPupupuUpdateFlowersBackStatusBefore);
    NDS_DIAG_RESET0(gNdsPupupuUpdateFlowersBackStatusAfterFinal);
    NDS_DIAG_RESET0(gNdsPupupuUpdateFlowersFrontStatusBefore);
    NDS_DIAG_RESET0(gNdsPupupuUpdateFlowersFrontStatusAfterFinal);
    NDS_DIAG_RESET0(gNdsPupupuUpdateMapGObjMaskBefore);
    NDS_DIAG_RESET0(gNdsPupupuUpdateMapGObjMaskAfter);
    NDS_DIAG_RESET0(gNdsPupupuUpdateGObjCountBefore);
    NDS_DIAG_RESET0(gNdsPupupuUpdateGObjCountAfter);
    NDS_DIAG_RESET0(gNdsPupupuUpdateVelPushCount);
    NDS_DIAG_RESET0(gNdsPupupuUpdateQuakeCount);
    NDS_DIAG_RESET0(gNdsPupupuUpdateParticleScriptCount);
    NDS_DIAG_RESET0(gNdsPupupuUpdateWindFGMCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomCameraCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDL0Size);
    NDS_DIAG_RESET0(gNdsOpeningRoomDL1Size);
    NDS_DIAG_RESET0(gNdsOpeningRoomGraphicsHeapSize);
    NDS_DIAG_RESET0(gNdsOpeningRoomRdpBufferSize);
    NDS_DIAG_RESET0(gNdsOpeningRoomMallocCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomPreAssetResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventTick);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventProbeMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventPencilsDObjOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventPencilsAnimOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventDataResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventDataMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventPencilsDObjEntries);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventPencilsDLPtrs);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventPencilsAnimJoints);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventRunResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomFirstEventDeferredMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomFighterDeferredResult);
    gNdsOpeningRoomFighterDeferredKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomTick380DeferredResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomTick380DeferredMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomTick450RunResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomTick450DeferredMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomTick500RunResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomTick500DeferredMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomTick560RunResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomTick560DeferredMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawResult);
    gNdsOpeningRoomDrawBlocker = NDS_OPENING_ROOM_DRAW_BLOCKER_NONE;
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTickCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawFrameCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawProbeCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawReuseCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawCameraCallbackCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawDisplayCallbackCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawDObjCallbackCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawFirstCameraMaskLow);
    gNdsOpeningRoomDrawFirstCameraPriority = 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraFlags = 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraXObjCount = 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraXObjKind0 = 0xffffffffu;
    gNdsOpeningRoomDrawFirstCameraXObjKind1 = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawFirstCameraViewportScaleX);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawFirstCameraViewportScaleY);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawFirstCameraViewportTransX);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawFirstCameraViewportTransY);
    NDS_DIAG_RESET0(gNdsRdpDefaultViewportSetCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsRdpDefaultViewportScaleX)
            NDS_DIAG_WORD0(gNdsRdpDefaultViewportScaleY)
            NDS_DIAG_WORD0(gNdsRdpDefaultViewportTransX)
            NDS_DIAG_WORD0(gNdsRdpDefaultViewportTransY)
            NDS_DIAG_WORD0(gNdsRdpDefaultViewportScaleZ)
            NDS_DIAG_WORD0(gNdsRdpDefaultViewportTransZ)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawFirstCameraNear100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawFirstCameraFar100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawFirstCameraFovY100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawFirstCameraEyeX100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawFirstCameraEyeY100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawFirstCameraEyeZ100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawFirstCameraAtX100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawFirstCameraAtY100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawFirstCameraAtZ100)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsOpeningRoomDrawFirstObjectDLLink = 0xffffffffu;
    gNdsOpeningRoomDrawFirstObjectID = 0xffffffffu;
    gNdsOpeningRoomDrawFirstObjectKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawFirstCallback);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawFirstDObjDL);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawFirstDObjMeta);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateCameraMaskLow);
    gNdsOpeningRoomDrawMaterialCandidateCameraPriority = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateObjectDLLink = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateObjectID = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateObjectKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateCallback);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateDObjDL);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateDObjMeta);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjEffectiveFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjMask);
    gNdsOpeningRoomDrawMaterialCandidateMObjTextureCurr = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjTextureNext = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjPaletteIndex = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjLfrac100);
    gNdsOpeningRoomDrawMaterialCandidateMObjFormat = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjSize = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjBlockFormat = 0xffffffffu;
    gNdsOpeningRoomDrawMaterialCandidateMObjBlockSize = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjTileWidth);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjTileHeight);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjScrollWidth);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjScrollHeight);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjScaleS100);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjScaleT100);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjTranslateS100);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjTranslateT100);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjSpriteArray);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjPaletteArray);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjSpriteCurr);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjSpriteNext);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialCandidateMObjPalettePtr);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialCandidateCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialMObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialSpriteArrayCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialSpriteCurrCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialSpriteNextCount);
    gNdsOpeningRoomDrawTextureMaterialObjectDLLink = 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialObjectID = 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialObjectKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialCallback);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialDObjDL);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialDObjMeta);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialMObjFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialMObjEffectiveFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialMObjMask);
    gNdsOpeningRoomDrawTextureMaterialMObjTextureCurr = 0xffffffffu;
    gNdsOpeningRoomDrawTextureMaterialMObjTextureNext = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialMObjSpriteArray);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialMObjSpriteCurr);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawTextureMaterialMObjSpriteNext);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchMObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchSegmentCommands);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchTableCommands);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchGeneratedCommands);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchFirstMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchFirstGeneratedCommands);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleS);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleT);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawMaterialBranchFirstTileUls)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawMaterialBranchFirstTileUlt)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawMaterialBranchFirstTileLrs)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawMaterialBranchFirstTileLrt)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawMaterialBranchFirstScrollUls)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawMaterialBranchFirstScrollUlt)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawMaterialBranchFirstScrollLrs)
            NDS_DIAG_WORD0(gNdsOpeningRoomDrawMaterialBranchFirstScrollLrt)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockTexels);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockDxt);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitResult);
    gNdsOpeningRoomDrawMaterialEmitBlocker =
        NDS_OPENING_ROOM_DRAW_MATERIAL_EMIT_BLOCKER_NONE;
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitUnsupportedMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitMObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitTableCommands);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitGeneratedCommands);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitHeapStart);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitBranchStart);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitHeapAfter);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitHeapBytes);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitFirstTableOp);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitFirstBranchOp0);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitFirstBranchOp1);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitFirstBranchOp2);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_0);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_0);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_1);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_1);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_2);
    NDS_DIAG_RESET0(gNdsOpeningRoomDrawMaterialEmitFirstBranchW1_2);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewResult);
    gNdsOpeningRoomDLPreviewBlocker =
        NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NONE;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewVertexCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTriangleCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewPixelCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewFirstOpcode);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewUnsupportedOpcode);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewUnsupportedCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewVertexCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTriangleCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewSyncCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewEndCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewBranchCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewBranchCallCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewBranchJumpCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewSegmentResolveCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewColorCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewPrimColor);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewOtherModeCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewFirstDL);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTransformMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewXObjCount);
    gNdsOpeningRoomDLPreviewFirstXObjKind = 0xffffffffu;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewTranslateX100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewTranslateY100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewTranslateZ100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewRotateX100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewRotateY100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewRotateZ100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewScaleX100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewScaleY100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewScaleZ100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewMinX)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewMaxX)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewMinY)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewMaxY)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewProjectionMask);
    gNdsOpeningRoomDLPreviewProjectionMode =
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_MODE_NONE;
    gNdsOpeningRoomDLPreviewProjectionBlocker =
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_NONE;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewProjectedVertexCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewProjectedTriangleCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewProjectedMinX)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewProjectedMaxX)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewProjectedMinY)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewProjectedMaxY)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewProjectedMinDepth100)
            NDS_DIAG_WORD0(gNdsOpeningRoomDLPreviewProjectedMaxDepth100)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsOpeningRoomDLPreviewFallbackAxis =
        NDS_OPENING_ROOM_DL_PREVIEW_FALLBACK_AXIS_XY;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewFallbackArea);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewGeometryCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewGeometryClearMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewGeometrySetMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewGeometryFinalMode);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewGeometryFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewGeometryPositiveWinding);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewGeometryNegativeWinding);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewGeometryZeroArea);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewGeometryDrawnTriangles);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureImage);
    gNdsOpeningRoomDLPreviewTextureFormat = 0xffffffffu;
    gNdsOpeningRoomDLPreviewTextureSize = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureImageWidth);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureTileWidth);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureTileHeight);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureTexelWidth);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureTexelHeight);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureLoadTexels);
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
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureSamplePixels);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureSetTileCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureCombineW0);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureCombineW1);
    gNdsOpeningRoomDLPreviewTextureCombineMode =
        NDS_OPENING_ROOM_DL_COMBINE_MODE_UNKNOWN;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureCombineFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureModulatedPixels);
    gNdsOpeningRoomDLPreviewTextureRenderTile = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureRenderTileLine);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureRenderTileTmem);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureRenderTilePalette);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureRenderTileCms);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureRenderTileCmt);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureRenderTileMasks);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureRenderTileMaskt);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureRenderTileShifts);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureRenderTileShiftt);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureRenderTileFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureReady);
    gNdsOpeningRoomDLPreviewTextureSampleMode =
        NDS_OPENING_ROOM_DL_TEXTURE_SAMPLE_NONE;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureScaleS);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureScaleT);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureLevel);
    gNdsOpeningRoomDLPreviewTextureTile = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureOn);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureXParam);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewTextureStateFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialEffectiveFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialMask);
    gNdsOpeningRoomDLPreviewMaterialTextureCurr = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialTextureNext = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialPaletteIndex = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialLfrac100);
    gNdsOpeningRoomDLPreviewMaterialFormat = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialSize = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialBlockFormat = 0xffffffffu;
    gNdsOpeningRoomDLPreviewMaterialBlockSize = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialTileWidth);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialTileHeight);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialScrollWidth);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialScrollHeight);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialScaleS100);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialScaleT100);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialTranslateS100);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialTranslateT100);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialSpriteCurr);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialSpriteNext);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialPalettePtr);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialBranchResult);
    gNdsOpeningRoomDLPreviewMaterialBranchBlocker =
        NDS_OPENING_ROOM_DL_PREVIEW_MATERIAL_BRANCH_BLOCKER_NONE;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialBranchCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialBranchPrimCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialBranchEndCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialBranchUnsupportedOp);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialBranchFirstOp);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialBranchSecondOp);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialBranchPrimColor);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialBranchPrimLod);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewMaterialBranchPrimM);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererParsedCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererStateCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererSkippedCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererRenderedCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureImage);
    gNdsOpeningRoomDLPreviewRendererTextureFormat = 0xffffffffu;
    gNdsOpeningRoomDLPreviewRendererTextureSize = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureImageWidth);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureLoadTexels);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureSetTileCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureStateFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureTileWidth);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureTileHeight);
    gNdsOpeningRoomDLPreviewRendererTextureRenderTile = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureRenderTileLine);
    NDS_DIAG_RESET0(gNdsOpeningRoomDLPreviewRendererTextureRenderTileFlags);
    gNdsOpeningRoomDLPreviewRendererTextureLoadBlockLrs = 0xffffffffu;
    gNdsOpeningRoomDLPreviewRendererTextureLoadBlockDxt = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeResult);
    gNdsOpeningRoomMaterialDLProbeBlocker =
        NDS_OPENING_ROOM_MATERIAL_DL_PROBE_BLOCKER_NONE;
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeFirstDL);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeVertexCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeTriangleCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeFirstOpcode);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeUnsupportedOpcode);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeVertexCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeTriangleCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeSyncCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeEndCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeBranchCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeOtherModeCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLProbeUnsupportedCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandResult);
    gNdsOpeningRoomMaterialDLExpandBlocker =
        NDS_OPENING_ROOM_MATERIAL_DL_EXPAND_BLOCKER_NONE;
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandFirstDL);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandFirstBranchDL);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandFirstResolvedBranchDL);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandVertexCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandTriangleCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandFirstOpcode);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandUnsupportedOpcode);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandVertexCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandTriangleCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandSyncCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandEndCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandBranchCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandBranchCallCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandBranchJumpCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandSegmentResolveCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandOtherModeCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandColorCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandUnsupportedCommandCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomMaterialDLExpandMaxDepth);
    NDS_DIAG_RESET0(sNdsOpeningRoomCurrentDrawCameraGObj);
    NDS_DIAG_RESET0(sNdsOpeningRoomPreviewCameraGObj);
    NDS_DIAG_RESET0(sNdsOpeningRoomFallbackPreviewCameraGObj);
    NDS_DIAG_RESET0(sNdsOpeningRoomFallbackPreviewGObj);
    NDS_DIAG_RESET0(sNdsOpeningRoomFallbackPreviewDObj);
    NDS_DIAG_RESET0(sNdsOpeningRoomFallbackPreviewDL);
    NDS_DIAG_RESET0(sNdsOpeningRoomMaterialPreviewCameraGObj);
    NDS_DIAG_RESET0(sNdsOpeningRoomMaterialPreviewGObj);
    NDS_DIAG_RESET0(sNdsOpeningRoomMaterialPreviewDObj);
    NDS_DIAG_RESET0(sNdsOpeningRoomMaterialPreviewDL);
    NDS_DIAG_RESET0(gNdsOpeningRoomOutsideAssetMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomOutsideDisplayListOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomOutsideCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomOutsideCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomOutsideCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomOutsideGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomOutsideDObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomOutsideXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomOutsideDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomHazeAssetMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomHazeDisplayListOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomHazeCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomHazeCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomHazeCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomHazeGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomHazeDObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomHazeXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomHazeDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightAssetMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightDisplayListOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightDObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomDeskAssetMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomDeskDObjOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomDeskCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomDeskCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomDeskCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomDeskGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomDeskDObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomDeskXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomDeskDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightEjectResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightEjectBeforeGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightEjectAfterGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomSunlightEjectUnlinkedMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomOverlayCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomOverlayDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomOverlayAlphaInit);
    NDS_DIAG_RESET0(gNdsOpeningRoomOverlayCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomOverlayEjectResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomOverlayEjectBeforeGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomOverlayEjectAfterGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomOverlayEjectUnlinkedMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCreateTick);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayDisplaySet);
    gNdsOpeningRoomCloseUpOverlayAlphaInit = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightAssetMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightDisplayListOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightMObjOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightMatAnimOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightCreateTick);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightDObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightMObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightAObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightProcessSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightMObjSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightMatAnimSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomSpotlightPositionSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraCObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraAObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraProcessSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraAnimSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraViewportSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene1CameraDLBufferSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraAssetMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraAnimOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraEjectResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraEjectMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraEjectBeforeGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraEjectAfterGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraEjectBeforeCameraCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraEjectAfterCameraCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraCObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraAObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraProcessSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraAnimSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraViewportSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomScene2CameraDLBufferSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCameraCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCameraCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCameraCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCameraGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCameraCObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCameraXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCameraDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomCloseUpOverlayCameraViewportSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomWallpaperCameraCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomWallpaperCameraCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomWallpaperCameraCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomWallpaperCameraGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomWallpaperCameraCObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomWallpaperCameraXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomWallpaperCameraDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomWallpaperCameraViewportSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraAssetMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraAnimOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraCObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraAObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraProcessSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraAnimSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCameraViewportSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoAssetMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoDObjOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoMObjOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoMatAnimOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoDObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoMObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoAObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoMObjSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoMatAnimSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoEjectResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoEjectBeforeGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoEjectAfterGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomLogoEjectUnlinkedMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowAssetMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowDisplayListOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowAnimOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowCreateGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowDObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowAObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowProcessSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowAnimSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowEjectResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowEjectBeforeGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowEjectAfterGObjCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomBossShadowEjectUnlinkedMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomPencilsCreateResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomPencilsCreateMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomPencilsGObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomPencilsDObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomPencilsXObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomPencilsAObjDelta);
    NDS_DIAG_RESET0(gNdsOpeningRoomPencilsProcessSet);
    NDS_DIAG_RESET0(gNdsOpeningRoomPencilsDisplaySet);
    NDS_DIAG_RESET0(gNdsOpeningRoomPencilsDObjTreeCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomPencilsAnimRootCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomControllerCheckCount);
    gNdsOpeningRoomPulledFighterKind = 0xffffffffu;
    gNdsOpeningRoomDroppedFighterKind = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomSkipToTitleCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocInitCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocLoadCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocFileMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocHeaderMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocPayloadMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocContentReady);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocFixupReady);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocBytesLoaded);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocLastFileID);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocLastSize);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocWordSwapMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocWordSwapCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocWordSwapFailCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocPointerFixupMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocPointerFixupCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocPointerFixupFailCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocSymbolResolveCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocSymbolResolveFailCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocSymbolProbeMask);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocLastSymbolOffset);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocMObjSubNormalizeCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocMObjSubNormalizeFailCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocMObjSubFirstFlags);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocMObjSubSourceResult);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocMObjSubTextureFlagCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocMObjSubZeroFlagCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocMObjSubPrimColorCount);
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocMObjSubLightCount);
    gNdsOpeningRoomRelocMObjSubFirstTextureOffset = 0xffffffffu;
    NDS_DIAG_RESET0(gNdsOpeningRoomRelocMObjSubFirstTextureFlags);

    ndsPlatformClearOriginalSpritePreview();
    NDS_DIAG_RESET0(sNdsRelocInitCount);
    NDS_DIAG_RESET0(gNdsLBFadeCreateCount);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(sNdsOpeningRoomPencilsCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomPencilsGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomPencilsDObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomPencilsXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomPencilsAObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomCloseUpOverlayCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomCloseUpOverlayGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomOutsideCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomOutsideGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomOutsideDObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomOutsideXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomHazeCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomHazeGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomHazeDObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomHazeXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomSunlightCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomSunlightGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomSunlightDObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomSunlightXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomDeskCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomDeskGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomDeskDObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomDeskXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomSpotlightCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomSpotlightGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomSpotlightDObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomSpotlightXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomSpotlightMObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomSpotlightAObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomBossShadowCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomBossShadowGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomBossShadowDObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomBossShadowXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomBossShadowAObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomScene1CameraCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomScene1CameraGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomScene1CameraCObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomScene1CameraXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomScene1CameraAObjsBefore)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    NDS_DIAG_RESET0(sNdsOpeningRoomScene2EjectMainCameraGObj);
    NDS_DIAG_RESET0(sNdsOpeningRoomScene2EjectFighterCameraGObj);
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD0(sNdsOpeningRoomScene2CameraEjectCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomScene2CameraCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomScene2CameraGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomScene2CameraCObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomScene2CameraXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomScene2CameraAObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomCloseUpOverlayCameraCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomCloseUpOverlayCameraGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomCloseUpOverlayCameraCObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomCloseUpOverlayCameraXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomWallpaperCameraCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomWallpaperCameraGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomWallpaperCameraCObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomWallpaperCameraXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoCameraCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoCameraGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoCameraCObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoCameraXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoCameraAObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoCountsCaptured)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoGObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoDObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoXObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoMObjsBefore)
            NDS_DIAG_WORD0(sNdsOpeningRoomLogoAObjsBefore)
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
}

extern void ndsMNVSModeRunStartTransitionProbe(void);
extern void ndsMNPlayersVSRunReadyTransitionProbe(void);
extern void ndsMNMapsRunSelectVSBattleProbe(void);
extern void scVSBattleFuncUpdate(void);
extern volatile u32 gNdsFtPoseEvalTick;
