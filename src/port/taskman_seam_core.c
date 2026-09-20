#include "nds_scene_harness_config.h"

#include <nds/nds_freeze_diagnostics.h>
#include <nds/nds_menu_shell.h>
#include <nds/nds_ifcommon_oam.h>
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
    gNdsSceneBoundaryResult = 0;
    gNdsSceneBoundaryKind = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStartupTaskmanResult, 0u),
            NDS_DIAG_WORD(gNdsStartupTaskmanSceneKind, 0u),
            NDS_DIAG_WORD(gNdsStartupTaskmanDL0Size, 0u),
            NDS_DIAG_WORD(gNdsStartupTaskmanDL1Size, 0u),
            NDS_DIAG_WORD(gNdsStartupTaskmanControllerSet, 0u),
            NDS_DIAG_WORD(gNdsStartupFuncStartResult, 0u),
            NDS_DIAG_WORD(gNdsStartupSkipAllowWait, 0u),
            NDS_DIAG_WORD(gNdsStartupProceedOpening, 0u),
            NDS_DIAG_WORD(gNdsStartupGObjCreateCount, 0u),
            NDS_DIAG_WORD(gNdsStartupCameraCreateCount, 0u),
            NDS_DIAG_WORD(gNdsStartupRelocInitCount, 0u),
            NDS_DIAG_WORD(gNdsStartupSpriteCreateCount, 0u),
            NDS_DIAG_WORD(gNdsStartupFadeCreateCount, 0u),
            NDS_DIAG_WORD(gNdsStartupWallpaperParentValid, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoPosX, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoPosY, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoFastcopyCleared, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoRelocResult, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoRelocSize, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoRelocWordSwapCount, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoRelocPointerFixupCount, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawResult, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsStartupLogoDrawBlocker = NDS_STARTUP_LOGO_BLOCKER_NONE;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStartupLogoDrawCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawWidth, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawHeight, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawFormat, 1u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawSize, 1u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawBitmaps, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawPixels, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawGObjID, 1u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawGObjObjKind, 1u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawSObjAttr, 1u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawTexshuf, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawTexshufSamples, 0u),
            NDS_DIAG_WORD(gNdsStartupLogoDrawVisibleSObjCount, 0u),
            NDS_DIAG_WORD(gNdsStartupActorFuncSet, 0u),
            NDS_DIAG_WORD(gNdsStartupWallpaperProcessKind, 1u),
            NDS_DIAG_WORD(gNdsStartupWallpaperProcessPriority, 1u),
            NDS_DIAG_WORD(gNdsStartupWallpaperDisplaySet, 0u),
            NDS_DIAG_WORD(gNdsStartupWallpaperCameraMaskLow, 0u),
            NDS_DIAG_WORD(gNdsStartupDefaultCameraColor, 0u),
            NDS_DIAG_WORD(gNdsTaskmanBridgeResult, 0u),
            NDS_DIAG_WORD(gNdsTaskmanContexts, 0u),
            NDS_DIAG_WORD(gNdsTaskmanTaskGfxNum, 0u),
            NDS_DIAG_WORD(gNdsTaskmanGraphicsHeapSize, 0u),
            NDS_DIAG_WORD(gNdsTaskmanRdpKind, 0u),
            NDS_DIAG_WORD(gNdsTaskmanRdpBufferSize, 0u),
            NDS_DIAG_WORD(gNdsTaskmanMallocCount, 0u),
            NDS_DIAG_WORD(gNdsStartupTaskmanMallocCount, 0u),
            NDS_DIAG_WORD(gNdsTaskmanGeneralHeapUsed, 0u),
            NDS_DIAG_WORD(gNdsTaskmanDLContextsValid, 0u),
            NDS_DIAG_WORD(gNdsTaskmanControllerAutoRead, 0u),
            NDS_DIAG_WORD(gNdsTaskmanSceneUpdateSet, 0u),
            NDS_DIAG_WORD(gNdsTaskmanSceneDrawSet, 0u),
            NDS_DIAG_WORD(gNdsTaskmanLightsSet, 0u),
            NDS_DIAG_WORD(gNdsTaskmanLoopReached, 0u),
            NDS_DIAG_WORD(gNdsTaskmanBoundedUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsTaskmanPostUpdateSkip, 0u),
            NDS_DIAG_WORD(gNdsTaskmanGObjThreadSleeps, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsOsGObjThreadProvisionCount = 0;
    gNdsOsGObjThreadProvisionFailCount = 0;
    gNdsOsThreadHeapCreateCount = 0;
    gNdsOsStartThreadNoEntryCount = 0;
    gNdsOsStartThreadCreateFailCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsTaskmanPostUpdateLogoPosX, 0u),
            NDS_DIAG_WORD(gNdsTaskmanPostUpdateLogoPosY, 0u),
            NDS_DIAG_WORD(gNdsTaskmanPostUpdateOpening, 0u),
            NDS_DIAG_WORD(gNdsTaskmanPostUpdateSceneKind, 0u),
            NDS_DIAG_WORD(gNdsTaskmanPostUpdateScenePrev, 0u),
            NDS_DIAG_WORD(gNdsTaskmanPostUpdateStatus, 0u),
            NDS_DIAG_WORD(gNdsTaskmanPostUpdateGObjCount, 0u),
            NDS_DIAG_WORD(gNdsTaskmanPostUpdateFadeCount, 0u),
            NDS_DIAG_WORD(gNdsTaskmanCleanupResult, 0u),
            NDS_DIAG_WORD(gNdsTaskmanCleanupQueuesEmpty, 0u),
            NDS_DIAG_WORD(gNdsTaskmanCleanupMode, 0u),
            NDS_DIAG_WORD(gNdsTaskmanReturnCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsTitleRelocResult, 0u),
            NDS_DIAG_WORD(gNdsTitlePreviewResult, 0u),
            NDS_DIAG_WORD(gNdsTitleDrawResult, 0u),
            NDS_DIAG_WORD(gNdsTitleSpriteNormalizeCount, 0u),
            NDS_DIAG_WORD(gNdsTitleSpriteNormalizeFailCount, 0u),
            NDS_DIAG_WORD(gNdsTitleFireSpriteNormalizeCount, 0u),
            NDS_DIAG_WORD(gNdsTitleFireSpriteNormalizeFailCount, 0u),
            NDS_DIAG_WORD(gNdsTitleDrawVisibleSObjCount, 0u),
            NDS_DIAG_WORD(gNdsTitleDrawRenderableSObjCount, 0u),
            NDS_DIAG_WORD(gNdsTitleDrawSObjCount, 0u),
            NDS_DIAG_WORD(gNdsTitleDrawPixels, 0u),
            NDS_DIAG_WORD(gNdsTitleDrawLastWidth, 0u),
            NDS_DIAG_WORD(gNdsTitleDrawLastHeight, 0u),
            NDS_DIAG_WORD(gNdsTitleDrawLastFormat, 0u),
            NDS_DIAG_WORD(gNdsTitleDrawLastSize, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalStartResult, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalFuncStartResult, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalSetupMask, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalLoadedFileCount, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalGObjCount, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalCameraCount, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalMainGObjID, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalTransitionGObjID, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalDeferredMask, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalLogoFireResult, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalLogoFireMask, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalLogoFireGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalLogoFireLinkID, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalLogoFireDLLinkID, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalLogoFireCameraMaskLo, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalLogoFireParticleBank, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalFireResult, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalFireMask, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalFireGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalFireSObjDelta, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalFireGObjFlags, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalFireSObjCount, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalFireFrames, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalFireAlpha, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalUpdateResult, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalLayout, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalTransitionTics, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalStartActorProcess, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalProceedScene, 0u),
            NDS_DIAG_WORD(gNdsTitleOriginalProceedWait, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalStartResult, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalFuncStartResult, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalRelocResult, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalSetupResult, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalSetupMask, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalLoadedFileCount, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalGObjCount, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalCameraCount, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalSObjCount, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalMainGObjID, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalCursorIndex, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalRule, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalTime, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalStock, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalButtonMask, 0u),
            NDS_DIAG_WORD(gNdsVSModeOriginalDeferredMask, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionResult, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionMask, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionInputMask, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionScenePrevBefore, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionSceneCurrBefore, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionScenePrevAfterTap, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionSceneCurrAfterTap, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionScenePrevFinal, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionSceneCurrFinal, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionExitInterrupt, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionTaskmanStatus, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionSavedRule, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionSavedTime, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionSavedStock, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionButtonMaskAfter, 0u),
            NDS_DIAG_WORD(gNdsVSModeStartTransitionCleanupCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    gNdsSCVSBattleOriginalFighterKinds = 0;
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStagePupupuRelocResult, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuRelocAssetMask, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuRelocDependencyMask, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuExternalFixupCount, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuExternalFixupFailCount, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuInternalFixupCount, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapHeaderOffset, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuGroundDataPtrReady, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuWallpaperPtrReady, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuGeometryPtrReady, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapNodesPtrReady, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuLightAngleXBits, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuLightAngleYBits, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuBGM, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjSourceCount, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjDecodedMask, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjDuplicateMask, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjUnalignedReadCount, 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjSourceIndices[0], 1u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjSourceIndices[1], 1u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjSourceIndices[2], 1u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjSourceIndices[3], 1u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjXs[0], 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjXs[1], 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjXs[2], 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjXs[3], 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjYs[0], 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjYs[1], 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjYs[2], 0u),
            NDS_DIAG_WORD(gNdsStagePupupuMapObjYs[3], 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    gNdsFighterNaturalCombatRollFrames = 0;
    gNdsFighterNaturalCombatRollStatus = 0;
    gNdsFighterNaturalCombatAppealFrames = 0;
    gNdsFighterNaturalCombatAppealStatus = 0;
    gNdsFighterProjectileProofResult = 0;
    gNdsFighterProjectileProofMask = 0;
    gNdsFighterProjectileProofActorSlot = 0;
    gNdsFighterProjectileProofActorKind = 0;
    gNdsFighterProjectileProofBPressFrames = 0;
    gNdsFighterProjectileProofSpecialStatusFrames = 0;
    gNdsFighterProjectileProofSpecialMotion = 0;
    gNdsFighterProjectileProofAccessoryFrames = 0;
    gNdsFighterProjectileProofFlag0Frames = 0;
    gNdsFighterDamageFireCallCount = 0;
    gNdsFighterEffectKindMask0 = 0;
    gNdsFighterEffectKindMask1 = 0;
    gNdsFighterEffectKindMask2 = 0;
    gNdsFighterEffectKindMask3 = 0;
    gNdsFighterModelPartSetCount = 0;
    gNdsFighterModelPartOnCount = 0;
    gNdsFighterModelPartResetCount = 0;
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
    gNdsFighterProjectileProofFireballIndex = -1;
    gNdsFighterProjectileProofFireballInitialVelXMilli = 0;
    gNdsFighterProjectileProofFireballInitialVelYMilli = 0;
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
    gNdsFighterSpecialsMarioHiDamageMax = 0;
    gNdsFighterSpecialsMarioLwPressFrames = 0;
    gNdsFighterSpecialsMarioLwFrames = 0;
    gNdsFighterSpecialsMarioAirLwFrames = 0;
    gNdsFighterSpecialsMarioLwDustEffectCount = 0;
    gNdsFighterSpecialsMarioLwWaitFrames = 0;
    gNdsFighterSpecialsMarioLwDamageMax = 0;
    gNdsFighterSpecialsFoxHiPressFrames = 0;
    gNdsFighterSpecialsFoxHiStartFrames = 0;
    gNdsFighterSpecialsFoxHiHoldFrames = 0;
    gNdsFighterSpecialsFoxHiTravelFrames = 0;
    gNdsFighterSpecialsFoxHiEndFrames = 0;
    gNdsFighterSpecialsFoxHiBoundFrames = 0;
    gNdsFighterSpecialsFoxHiWaitFrames = 0;
    gNdsFighterSpecialsFoxHiRootYMilli = 0;
#if NDS_P2_DONKEY
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsSlot, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNChargePressFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNStartFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNLoopFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNStorePressFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNStoredChargeMax, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNStoredWaitFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNResumePressFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNReleaseTapFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNEndFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNReleaseChargeMax, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNPassiveResetFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsNReleaseWaitFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsHiPressFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsHiFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsHiGroundGAFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsHiWaitFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsLwPressFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsLwStartFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsLwLoopFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsLwRepeatPressFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsLwLoopFlagFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsLwEndFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterDonkeySpecialsLwWaitFrames, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
#endif
#if NDS_P2_SAMUS
    gNdsFighterSamusSpecialsSlot = 0;
    gNdsFighterSamusSpecialsNPressFrames = 0;
    gNdsFighterSamusSpecialsNStartFrames = 0;
    gNdsFighterSamusSpecialsNLoopFrames = 0;
    gNdsFighterSamusSpecialsNChargeMax = 0;
    gNdsFighterSamusSpecialsNFullWaitFrames = 0;
    gNdsFighterSamusSpecialsNReleasePressFrames = 0;
    gNdsFighterSamusSpecialsNEndFrames = 0;
    gNdsFighterSamusSpecialsNReleaseWaitFrames = 0;
    gNdsFighterSamusSpecialsLwPressFrames = 0;
    gNdsFighterSamusSpecialsLwFrames = 0;
    gNdsFighterSamusSpecialsLwWaitFrames = 0;
#endif
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterNaturalMovesetVictimRootYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableResult, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableMask, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableVictimSlot, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableVictimStockStart, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableVictimStockFinal, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableBattleStockStart, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableBattleStockFinal, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFallsStart, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFallsFinal, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableDeadFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableRebirthDownFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableRebirthStandFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableRebirthWaitFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFallAfterRebirthFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableWaitAfterRebirthFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalStatus, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalGA, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalFloor, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalIsRebirth, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalIsGhost, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalCameraMode, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableKOStickFrames, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableMapCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableMapHitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableMapFloorHitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableMapCliffHitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableMapCeilHitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableMapLastMaskStat, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableMapLastMaskCurr, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalVelXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalVelYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterBattlePlayableFinalFloorDistMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFTComputerStartXMilli, 0u),
            NDS_DIAG_WORD(gNdsFTComputerMinXMilli, 0u),
            NDS_DIAG_WORD(gNdsFTComputerMaxXMilli, 0u),
            NDS_DIAG_WORD(gNdsFTComputerFinalXMilli, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingResult, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingMode, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingLogicFrames, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPresentedFrames, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingDrawCalls, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingTimerTicks, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPresentFpsX10, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingLogicFpsX10, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingVBlankStart, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingVBlanks, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingRestartRequested, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPresentIntervalMin, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPresentIntervalMax, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPresentIntervalBucket[0], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPresentIntervalBucket[1], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPresentIntervalBucket[2], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPresentIntervalBucket[3], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPresentIntervalBucket[4], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPresentIntervalBucket[5], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingCadenceViolationCount, 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPhasePresentCount[0], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPhasePresentCount[1], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPhasePresentCount[2], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPhasePresentCount[3], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPhasePresentCount[4], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPhaseSlipCount[0], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPhaseSlipCount[1], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPhaseSlipCount[2], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPhaseSlipCount[3], 0u),
            NDS_DIAG_WORD(gNdsBattlePlayablePacingPhaseSlipCount[4], 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsRendererProfileProjectionM00, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileProjectionM11, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileProjectionM22, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileProjectionM32, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileModelviewM00, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileModelviewM11, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileModelviewM22, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileModelviewM30, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileModelviewM31, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileModelviewM32, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileRawVertexMinX, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileRawVertexMaxX, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileRawVertexMinY, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileRawVertexMaxY, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileRawVertexMinZ, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileRawVertexMaxZ, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileHWVertexMinX, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileHWVertexMaxX, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileHWVertexMinY, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileHWVertexMaxY, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileHWVertexMinZ, 0u),
            NDS_DIAG_WORD(gNdsRendererProfileHWVertexMaxZ, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    gNdsIFCommonHUDP2DamageCurrent = 0;
    gNdsIFCommonHUDP3DamageCurrent = 0;
    gNdsIFCommonHUDP0DamageMax = 0;
    gNdsIFCommonHUDP1DamageMax = 0;
    gNdsIFCommonHUDP2DamageMax = 0;
    gNdsIFCommonHUDP3DamageMax = 0;
    gNdsIFCommonHUDP0DigitCount = 0;
    gNdsIFCommonHUDP1DigitCount = 0;
    gNdsIFCommonHUDP2DigitCount = 0;
    gNdsIFCommonHUDP3DigitCount = 0;
    gNdsIFCommonHUDP0Digits = 0;
    gNdsIFCommonHUDP1Digits = 0;
    gNdsIFCommonHUDP2Digits = 0;
    gNdsIFCommonHUDP3Digits = 0;
    gNdsIFCommonHUDP0StockCurrent = 0;
    gNdsIFCommonHUDP1StockCurrent = 0;
    gNdsIFCommonHUDP2StockCurrent = 0;
    gNdsIFCommonHUDP3StockCurrent = 0;
    gNdsIFCommonHUDP0StockMin = 0;
    gNdsIFCommonHUDP1StockMin = 0;
    gNdsIFCommonHUDP2StockMin = 0;
    gNdsIFCommonHUDP3StockMin = 0;
    gNdsIFCommonHUDP0StockMax = 0;
    gNdsIFCommonHUDP1StockMax = 0;
    gNdsIFCommonHUDP2StockMax = 0;
    gNdsIFCommonHUDP3StockMax = 0;
    gNdsIFCommonHUDActivePlayerMask = 0;
    gNdsIFCommonHUDShowDamageMask = 0;
    gNdsIFCommonHUDDamageFlashMask = 0;
    gNdsIFCommonHUDSingleStockMask = 0;
    gNdsIFCommonHUDCPUPlayerMask = 0;
    gNdsIFCommonHUDP0FighterKind = 0;
    gNdsIFCommonHUDP1FighterKind = 0;
    gNdsIFCommonHUDP2FighterKind = 0;
    gNdsIFCommonHUDP3FighterKind = 0;
    gNdsIFCommonHUDP0Level = 0;
    gNdsIFCommonHUDP1Level = 0;
    gNdsIFCommonHUDP2Level = 0;
    gNdsIFCommonHUDP3Level = 0;
    gNdsIFCommonHUDP0Costume = 0;
    gNdsIFCommonHUDP1Costume = 0;
    gNdsIFCommonHUDP2Costume = 0;
    gNdsIFCommonHUDP3Costume = 0;
    gNdsIFCommonHUDP0LowerStock = 0;
    gNdsIFCommonHUDP1LowerStock = 0;
    gNdsIFCommonHUDP2LowerStock = 0;
    gNdsIFCommonHUDP3LowerStock = 0;
    gNdsIFCommonHUDTimeRemain = 0;
    gNdsIFCommonHUDTimerLimit = 0;
    gNdsIFCommonHUDTimerStarted = 0;
    gNdsIFCommonHUDTimerVisible = 0;
    gNdsIFCommonHUDGameStatus = 0;
    gNdsIFCommonHUDLowerRouteMask = 0;
    gNdsIFCommonHUDLowerRouteCount = 0;
    gNdsIFCommonHUDLowerTimerRouteCount = 0;
    gNdsIFCommonHUDLowerStockRouteCount = 0;
    gNdsIFCommonHUDLowerDamageRouteCount = 0;
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterInitDamageCollMask, 0u),
            NDS_DIAG_WORD(gNdsFighterInitDamageCollNormalMask, 0u),
            NDS_DIAG_WORD(gNdsFighterInitDamageCollJointMask, 0u),
            NDS_DIAG_WORD(gNdsFighterInitDamageCollHalfSizeMask, 0u),
            NDS_DIAG_WORD(gNdsFighterInitDamageCollPartsMask, 0u),
            NDS_DIAG_WORD(gNdsFighterInitDamageCollMatrixMask, 0u),
            NDS_DIAG_WORD(gNdsFighterInitDamageCollScaleMask, 0u),
            NDS_DIAG_WORD(gNdsFighterInitP0DamageCollCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterDisplayP0DObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1DObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0MObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1MObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0AObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1AObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0DLReadyCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1DLReadyCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0PartsPtrCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1PartsPtrCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP0RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayP1RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayDrawCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayMatrixCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayGameplayUpdateCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterMarioFoxDLScanResult = 0;
    gNdsFighterMarioFoxDLScanSafeResult = 0;
    gNdsFighterMarioFoxDLScanMask = 0;
    gNdsFighterMarioFoxDLScanDeferredMask = 0;
    gNdsFighterMarioFoxDLScanCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterDLScanP0FirstDL, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1FirstDL, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0AssetID, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1AssetID, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0Offset, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1Offset, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0DObjIndex, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1DObjIndex, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0Blocker, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1Blocker, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0CommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1CommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0FirstOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1FirstOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0UnsupportedOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1UnsupportedOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0UnsupportedCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1UnsupportedCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0VertexCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1VertexCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0TriangleCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1TriangleCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0VertexCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1VertexCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0TriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1TriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0EndCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1EndCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0BranchCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1BranchCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0SegmentResolveCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1SegmentResolveCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0TextureMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1TextureMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0OtherModeCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1OtherModeCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0CullCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1CullCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0StateCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1StateCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0SkipCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1SkipCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0RenderCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1RenderCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0MaxDepthSeen, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1MaxDepthSeen, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP0RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanP1RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanDrawCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanMatrixCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanGameplayUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanRangeRejectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLScanBranchResolveCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterMarioFoxDLExecResult = 0;
    gNdsFighterMarioFoxDLExecSafeResult = 0;
    gNdsFighterMarioFoxDLExecMask = 0;
    gNdsFighterMarioFoxDLExecDeferredMask = 0;
    gNdsFighterMarioFoxDLExecCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterDLExecP0Blocker, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1Blocker, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0CommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1CommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0FirstOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1FirstOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0UnsupportedOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1UnsupportedOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0UnsupportedCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1UnsupportedCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0VertexCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1VertexCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0VertexDecodedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1VertexDecodedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0VertexValidMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1VertexValidMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0TriangleCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1TriangleCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0TriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1TriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0TriangleValidCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1TriangleValidCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0MinX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0MaxX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0MinY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0MaxY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0MinZ, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0MaxZ, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1MinX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1MaxX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1MinY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1MaxY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1MinZ, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1MaxZ, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0OtherModeCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1OtherModeCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0CullCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1CullCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0StateCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1StateCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0SkipCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1SkipCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0RenderCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1RenderCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0BranchCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1BranchCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0SegmentResolveCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1SegmentResolveCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0TextureMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1TextureMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP0RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecP1RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecDrawCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecMatrixCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecGameplayUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecRangeRejectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLExecVertexRangeRejectCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterMarioFoxDLDrawResult = 0;
    gNdsFighterMarioFoxDLDrawSafeResult = 0;
    gNdsFighterMarioFoxDLDrawMask = 0;
    gNdsFighterMarioFoxDLDrawDeferredMask = 0;
    gNdsFighterMarioFoxDLDrawCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterDLDrawPreviewWidth, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawPreviewHeight, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawPreviewPitch, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawPreviewReady, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawPreviewCommitBefore, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawPreviewCommitAfter, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawPreviewCommitDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0Blocker, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1Blocker, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0CommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1CommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0FirstOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1FirstOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0UnsupportedOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1UnsupportedOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0UnsupportedCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1UnsupportedCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0VertexDecodedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1VertexDecodedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0TriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1TriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0TriangleValidCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1TriangleValidCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0TriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1TriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0RealTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1RealTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0MarkerTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1MarkerTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0PixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1PixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawTotalPixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0Axis, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1Axis, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0Area, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1Area, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0MinA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0MaxA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0MinB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0MaxB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1MinA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1MaxA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1MinB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1MaxB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0ScreenMinX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0ScreenMaxX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0ScreenMinY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0ScreenMaxY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1ScreenMinX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1ScreenMaxX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1ScreenMinY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1ScreenMaxY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP0RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawP1RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawDrawCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawMatrixCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawGameplayUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawRangeRejectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLDrawVertexRangeRejectCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterMarioFoxDLMultiDrawResult = 0;
    gNdsFighterMarioFoxDLMultiDrawSafeResult = 0;
    gNdsFighterMarioFoxDLMultiDrawMask = 0;
    gNdsFighterMarioFoxDLMultiDrawDeferredMask = 0;
    gNdsFighterMarioFoxDLMultiDrawCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawPreviewWidth, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawPreviewHeight, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawPreviewPitch, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawPreviewReady, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawPreviewCommitBefore, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawPreviewCommitAfter, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawPreviewCommitDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0CandidateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1CandidateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0SelectedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1SelectedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0AttemptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1AttemptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0CleanCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1CleanCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0DrawnDObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1DrawnDObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0FailedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1FailedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0SelectedIndexMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1SelectedIndexMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0FirstBlocker, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1FirstBlocker, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0BlockerMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1BlockerMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0CommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1CommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0FirstOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1FirstOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0UnsupportedOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1UnsupportedOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0UnsupportedCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1UnsupportedCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0VertexDecodedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1VertexDecodedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0TriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1TriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0TriangleValidCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1TriangleValidCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0TriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1TriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0RealTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1RealTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0MarkerTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1MarkerTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0PixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1PixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawTotalPixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0Axis, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1Axis, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0Area, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1Area, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0MinA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0MaxA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0MinB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0MaxB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1MinA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1MaxA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1MinB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1MaxB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0ScreenMinX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0ScreenMaxX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0ScreenMinY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0ScreenMaxY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1ScreenMinX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1ScreenMaxX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1ScreenMinY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1ScreenMaxY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP0RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawP1RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawDrawCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawMatrixCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawGameplayUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawRangeRejectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLMultiDrawVertexRangeRejectCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterMarioFoxDLAllDrawResult = 0;
    gNdsFighterMarioFoxDLAllDrawSafeResult = 0;
    gNdsFighterMarioFoxDLAllDrawMask = 0;
    gNdsFighterMarioFoxDLAllDrawDeferredMask = 0;
    gNdsFighterMarioFoxDLAllDrawCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterDLAllDrawDisplayCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0DisplayCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1DisplayCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawPreviewWidth, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawPreviewHeight, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawPreviewPitch, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawPreviewReady, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawPreviewCommitBefore, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawPreviewCommitAfter, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawPreviewCommitDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0CandidateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1CandidateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0SelectedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1SelectedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawCandidateHighWater, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawSelectedHighWater, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawTruncateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawSelectedOverflowCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFTManagerFigatreeSlotKindCount = 0;
    gNdsFTManagerFigatreeSlotKindBytes = 0;
    gNdsFTManagerFigatreeSlotKindMin = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0AttemptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1AttemptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0CleanCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1CleanCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0DrawnDObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1DrawnDObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0FailedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1FailedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0SelectedIndexMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1SelectedIndexMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0FirstBlocker, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1FirstBlocker, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0BlockerMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1BlockerMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0CommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1CommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0FirstOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1FirstOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0UnsupportedOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1UnsupportedOpcode, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0UnsupportedCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1UnsupportedCommandCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0VertexDecodedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1VertexDecodedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0MatrixMvpRecalcCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1MatrixMvpRecalcCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0MatrixMoveWordCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1MatrixMoveWordCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0HardwareTriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawSlotTriangleMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1HardwareTriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0HardwareOracleTriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1HardwareOracleTriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0HardwareOracleRejectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1HardwareOracleRejectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0HardwareMatrixSeedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1HardwareMatrixSeedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawHardwareTextureBindCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawHardwareTextureUploadCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawHardwareTextureReadyCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawHardwareTextureRejectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawHardwareTextureFormatMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawHardwareTextureMaxWidth, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawHardwareTextureMaxHeight, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0TriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1TriangleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0TriangleValidCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1TriangleValidCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0TriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1TriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0RealTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1RealTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0MarkerTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1MarkerTriangleDrawnCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0PixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1PixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawTotalPixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0Axis, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1Axis, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0Area, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1Area, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0MinA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0MaxA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0MinB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0MaxB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1MinA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1MaxA, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1MinB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1MaxB, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0ScreenMinX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0ScreenMaxX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0ScreenMinY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0ScreenMaxY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1ScreenMinX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1ScreenMaxX, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1ScreenMinY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1ScreenMaxY, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1StatusAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1MotionAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1GAAfter, 1u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP0RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1RootXBeforeBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawP1RootXAfterBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawDrawCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawMatrixCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawGameplayUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawRangeRejectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDLAllDrawVertexRangeRejectCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterWalkP0GroundVelBeforeMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkP1GroundVelBeforeMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkP0GroundVelAfterMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkP1GroundVelAfterMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkP0AirVelXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkP1AirVelXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkP0AirVelYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkP1AirVelYMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterWalkLoopP0GroundVelStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkLoopP1GroundVelStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkLoopP0GroundVelAfterHeldMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkLoopP1GroundVelAfterHeldMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkLoopP0GroundVelAfterSettleMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkLoopP1GroundVelAfterSettleMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkLoopP0AirVelXAfterHeldMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkLoopP1AirVelXAfterHeldMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkLoopP0AirVelYAfterHeldMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterWalkLoopP1AirVelYAfterHeldMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterDashRunWaitInterruptCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGroundCheckCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunOriginalDashCheckCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunOriginalDashCheckSuccessCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack1CheckCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack1CheckSuccessCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack100StartCheckCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackDashCheckCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackDashCheckSuccessCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDashSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunRunSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunRunBrakeSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack11SetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack12SetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack13SetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack100StartSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack100LoopSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackDashSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDashInterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunRunInterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunRunBrakeInterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDashPhysicsCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunRunPhysicsCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunRunBrakePhysicsCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDashMapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunRunMapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunRunBrakeMapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunSafeFloorCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFallBreakSafeCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDeferredInterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainDashStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainRunStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainRunBrakeStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainAttack11StatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainAttack12StatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainAttack13StatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainAttack100StartStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainAttack100LoopStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainAttackDashStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAnimEventsCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGroundVelFrictionCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGroundVelTransferAirCount, 0u),
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
            NDS_DIAG_WORD(gNdsFighterDashRunAttack11CallbackMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack11TickMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack11WaitProcMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack12CallbackMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack12GotoMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack13CallbackMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack13GotoMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack100StartCallbackMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack100StartGotoMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack100LoopCallbackMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack100LoopGotoMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttack100LoopTickMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackAnimEventsMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventScriptMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventNoHitMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventCommandMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventParseCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastPlayer, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastStatus, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastState, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastAttackID, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastGroupID, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastJointID, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastDamage, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastSize, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastOffsetX, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastOffsetY, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastOffsetZ, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastAngle, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastKBG, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastKBW, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastBKB, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastShield, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventLastFlags, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionState, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionAttackID, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionJointID, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionX, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionY, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionZ, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionMatrixFlag, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackEventPositionMatrixValue, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusLevel, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusIndex, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusGround, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusAir, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageStatusElectric, 1u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageSetupMask, 0u),
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
            NDS_DIAG_WORD(gNdsFighterDashRunDamageSetupVelGroundMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageSetupVelAirXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageSetupVelAirYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDamageSetupVelPhysicsMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardCheckCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardCheckSuccessCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainGuardOnStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardSetOffSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainGuardSetOffStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardAnimEventsMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardEffectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardFGMCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardLastFGM, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0StatusGuardOn, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusGuardOn, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0MotionGuardOn, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionGuardOn, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardCallbackMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardStateMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardSetOffMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardSetOffCallbackMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardSetOffFramesMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGuardSetOffVelMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunEscapeCheckCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunEscapeCheckSuccessCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunEscapeSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunFtMainEscapeStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunEscapeCallbackMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunEscapeStateMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunEscapeTickMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunEscapeInterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunEscapePhysicsCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunEscapeMapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0StatusEscape, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StatusEscape, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0MotionEscape, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1MotionEscape, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0EscapeItemThrowBuffer, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1EscapeItemThrowBuffer, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackDashCallbackMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackDashTickMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunAttackDashRunProcMask, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0TapStickXAfterDash, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1TapStickXAfterDash, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0LR, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1LR, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0StickX, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1StickX, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0GroundVelRunMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1GroundVelRunMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0GroundVelBrakeMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1GroundVelBrakeMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP0RootDirectionOK, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunP1RootDirectionOK, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunRootYDriftCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGADriftCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDeniedStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunUnexpectedStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunProcessAttachCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDisplayProbeCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunGameplayUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunDrawCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDashRunMatrixCallCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterJumpP0VelXInitialMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterJumpP1VelXInitialMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterJumpP0VelYInitialMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterJumpP1VelYInitialMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterJumpP0VelXAfterMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterJumpP1VelXAfterMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterJumpP0VelYAfterMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterJumpP1VelYAfterMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterLandingP0FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP1FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP0RootYFallStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP1RootYFallStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP0RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP1RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP0RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP1RootDeltaXMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterLandingP0RootDirectionOK = 0;
    gNdsFighterLandingP1RootDirectionOK = 0;
    gNdsFighterLandingP0RootFloorOK = 0;
    gNdsFighterLandingP1RootFloorOK = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterLandingP0VelYFallStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP1VelYFallStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP0VelYBeforeLandingMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP1VelYBeforeLandingMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP0GroundVelAfterLandingMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP1GroundVelAfterLandingMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP0GroundVelAfterWaitMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterLandingP1GroundVelAfterWaitMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterProcessLoopP0FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP1FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP0RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP1RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP0RootXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP1RootXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP0RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP1RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP0RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP1RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP0RootRiseMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP1RootRiseMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterProcessLoopP0RootDirectionOK = 0;
    gNdsFighterProcessLoopP1RootDirectionOK = 0;
    gNdsFighterProcessLoopP0FloorOK = 0;
    gNdsFighterProcessLoopP1FloorOK = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterProcessLoopP0GroundVelFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP1GroundVelFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP0AirVelXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP1AirVelXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP0AirVelYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterProcessLoopP1AirVelYFinalMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP0FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP1FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP0RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP1RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP0RootXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP1RootXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP0RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP1RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP0RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP1RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP0RootRiseMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP1RootRiseMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterSchedulerLoopP0RootDirectionOK = 0;
    gNdsFighterSchedulerLoopP1RootDirectionOK = 0;
    gNdsFighterSchedulerLoopP0FloorOK = 0;
    gNdsFighterSchedulerLoopP1FloorOK = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP0GroundVelFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP1GroundVelFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP0AirVelXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP1AirVelXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP0AirVelYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterSchedulerLoopP1AirVelYFinalMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterControllerLoopPrepared, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopFrameMax, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopUpdateMax, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopTaskmanUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopVSBattleUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopBaseVSBattleUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopSchedulerUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopSYReadCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopSYUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopGObjCountBefore, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopGObjCountAfter, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0ProcessAttachCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1ProcessAttachCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopProcessAttachEscapeCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0GObjProcessRunCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1GObjProcessRunCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0ProcCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1ProcCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0PlaybackApplyCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1PlaybackApplyCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0ControllerToFTInputCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1ControllerToFTInputCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0DirectFTInputWriteCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1DirectFTInputWriteCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0ButtonTapMask, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1ButtonTapMask, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0ButtonHoldMask, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1ButtonHoldMask, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0ButtonReleaseMask, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1ButtonReleaseMask, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0LastStickX, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1LastStickX, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0LastStickY, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1LastStickY, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0TapStickXMin, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1TapStickXMin, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0TapStickYMin, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1TapStickYMin, 1u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0DashTapEligibleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1DashTapEligibleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0JumpButtonTapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1JumpButtonTapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0FrameCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1FrameCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0Completed, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1Completed, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0StatusVisitMask, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1StatusVisitMask, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0TransitionMask, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1TransitionMask, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0WaitVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1WaitVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0WalkVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1WalkVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0DashVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1DashVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0RunVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1RunVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0RunBrakeVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1RunBrakeVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0KneeBendVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1KneeBendVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0JumpVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1JumpVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0FallVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1FallVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0LandingVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1LandingVisitCount, 0u),
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
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0RootXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1RootXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0RootRiseMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1RootRiseMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0RootDirectionOK, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1RootDirectionOK, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0FloorOK, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1FloorOK, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0GroundVelFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1GroundVelFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0AirVelXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1AirVelXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0AirVelYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1AirVelYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0UpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1UpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0InterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1InterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0PhysicsCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1PhysicsCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0IntegrateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1IntegrateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP0MapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopP1MapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopFallDetectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopLandingDetectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopSetGroundCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopSetAirCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopWaitSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopRunBrakeEndCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopJumpAnimEndCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopLandingEndCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopDeferredInterruptCheckCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopUnexpectedStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopDeniedStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopDisplayProbeCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopGameplayUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopDrawCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopMatrixCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopRootYDriftCount, 0u),
            NDS_DIAG_WORD(gNdsFighterControllerLoopGADriftCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP0RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP1RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP0RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP1RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP0RootRiseMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP1RootRiseMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP0RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP1RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP0FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP1FloorYMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP0ScreenXStart, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP1ScreenXStart, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP0ScreenXFinal, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP1ScreenXFinal, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP0ScreenXDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP1ScreenXDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP0ScreenYFloor, 0u),
            NDS_DIAG_WORD(gNdsFighterPreviewLoopP1ScreenYFloor, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopPrepared, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopFrameMax, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopUpdateMax, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopTaskmanUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopVSBattleUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopBaseVSBattleUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopRunAllCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopSYReadCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopSYUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopGObjCountBefore, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopGObjCountAfter, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopOldProcessPauseCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopNonTargetGObjVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopNonTargetProcessPauseCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopTargetProcessPreserveCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ProcessAttachCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ProcessAttachCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopProcessAttachEscapeCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0GObjProcessRunCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1GObjProcessRunCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ProcCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ProcCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0PlaybackApplyCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1PlaybackApplyCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ControllerToFTInputCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ControllerToFTInputCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0DirectFTInputWriteCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1DirectFTInputWriteCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ButtonTapMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ButtonTapMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ButtonHoldMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ButtonHoldMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0LastStickX, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1LastStickX, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0LastStickY, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1LastStickY, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0DashTapEligibleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1DashTapEligibleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0JumpButtonTapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1JumpButtonTapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0FrameCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1FrameCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0Completed, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1Completed, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0StatusVisitMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1StatusVisitMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0TransitionMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1TransitionMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0WaitVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1WaitVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0WalkVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1WalkVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0DashVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1DashVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0RunVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1RunVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0RunBrakeVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1RunBrakeVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0KneeBendVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1KneeBendVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0JumpVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1JumpVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0FallVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1FallVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0LandingVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1LandingVisitCount, 0u),
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
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0RootRiseMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1RootRiseMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0RootDirectionOK, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1RootDirectionOK, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0FloorOK, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1FloorOK, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0InterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1InterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0PhysicsCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1PhysicsCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0IntegrateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1IntegrateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0MapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1MapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopPreviewWidth, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopPreviewHeight, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopPreviewPitch, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopPreviewReady, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopPreviewCommitBefore, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopPreviewCommitAfter, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopPreviewCommitDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopDrawFrameCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopDisplayCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0DisplayCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1DisplayCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0CandidateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1CandidateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0DrawnDObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1DrawnDObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0PixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1PixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopTotalPixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ScreenXStart, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ScreenXStart, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ScreenXFinal, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ScreenXFinal, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ScreenXDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ScreenXDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ScreenYFloor, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ScreenYFloor, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterGCRunAllLoopP0ScreenYMin = 0x7fffffff;
    gNdsFighterGCRunAllLoopP1ScreenYMin = 0x7fffffff;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP0ScreenRise, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopP1ScreenRise, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopFallDetectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopLandingDetectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopSetGroundCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopSetAirCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopWaitSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopRunBrakeEndCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopJumpAnimEndCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopLandingEndCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopDeferredInterruptCheckCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopUnexpectedStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopDeniedStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopDisplayProbeCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopGameplayUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopDrawCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopMatrixCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopRootYDriftCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCRunAllLoopGADriftCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterMarioFoxGCDrawAllLoopResult = 0;
    gNdsFighterMarioFoxGCDrawAllLoopSafeResult = 0;
    gNdsFighterMarioFoxGCDrawAllLoopMask = 0;
    gNdsFighterMarioFoxGCDrawAllLoopDeferredMask = 0;
    gNdsFighterMarioFoxGCDrawAllLoopCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopPrepared, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopFrameMax, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopUpdateMax, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopTaskmanUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopVSBattleUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopBaseVSBattleUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopDrawAllCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopCameraCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopCapturedDisplayCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopNonTargetDisplayCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopRunAllCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopSYReadCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopSYUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopGObjCountBefore, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopGObjCountAfter, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopGObjDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopOldProcessPauseCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopNonTargetGObjVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopNonTargetProcessPauseCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopTargetProcessPreserveCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ProcessAttachCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ProcessAttachCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopProcessAttachEscapeCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0GObjProcessRunCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1GObjProcessRunCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ProcCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ProcCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0PlaybackApplyCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1PlaybackApplyCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ControllerToFTInputCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ControllerToFTInputCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0DirectFTInputWriteCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1DirectFTInputWriteCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ButtonTapMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ButtonTapMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ButtonHoldMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ButtonHoldMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0LastStickX, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1LastStickX, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0LastStickY, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1LastStickY, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0DashTapEligibleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1DashTapEligibleCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0JumpButtonTapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1JumpButtonTapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0FrameCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1FrameCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0Completed, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1Completed, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0StatusVisitMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1StatusVisitMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0TransitionMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1TransitionMask, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0WaitVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1WaitVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0WalkVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1WalkVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0DashVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1DashVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0RunVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1RunVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0RunBrakeVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1RunBrakeVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0KneeBendVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1KneeBendVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0JumpVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1JumpVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0FallVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1FallVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0LandingVisitCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1LandingVisitCount, 0u),
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
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1RootXStartMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1RootDeltaXMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0RootRiseMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1RootRiseMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0RootDirectionOK, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1RootDirectionOK, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0FloorOK, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1FloorOK, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0InterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1InterruptCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0PhysicsCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1PhysicsCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0IntegrateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1IntegrateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0MapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1MapCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopPreviewWidth, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopPreviewHeight, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopPreviewPitch, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopPreviewReady, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopPreviewCommitBefore, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopPreviewCommitAfter, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopPreviewCommitDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopDrawFrameCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopDisplayCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0DisplayCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1DisplayCallbackCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0CandidateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1CandidateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0DrawnDObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1DrawnDObjCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0PixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1PixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopTotalPixelCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ColorChecksum, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ScreenXStart, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ScreenXStart, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ScreenXFinal, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ScreenXFinal, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ScreenXDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ScreenXDelta, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ScreenYFloor, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ScreenYFloor, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterGCDrawAllLoopP0ScreenYMin = 0x7fffffff;
    gNdsFighterGCDrawAllLoopP1ScreenYMin = 0x7fffffff;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP0ScreenRise, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopP1ScreenRise, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopFallDetectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopLandingDetectCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopSetGroundCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopSetAirCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopWaitSetStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopRunBrakeEndCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopJumpAnimEndCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopLandingEndCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopDeferredInterruptCheckCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopUnexpectedStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopDeniedStatusCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopDisplayProbeCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopGameplayUpdateCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopDrawCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopMatrixCallCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopRootYDriftCount, 0u),
            NDS_DIAG_WORD(gNdsFighterGCDrawAllLoopGADriftCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsFighterDisplayContractSelectedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractHiddenCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractNoTextureCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractSubmittedCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractGeometryMode, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractCycleType, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractRenderMode, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractLightCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractLightDirectionCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractBoundsPassCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractBoundsFailCount, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractBoundsXBits, 0u),
            NDS_DIAG_WORD(gNdsFighterDisplayContractBoundsYBits, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStageCollisionLoopP0FloorAngleX1000, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP0FloorAngleY1000, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP1FloorAngleX1000, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP1FloorAngleY1000, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP0EdgeLX, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP0EdgeLY, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP0EdgeRX, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP0EdgeRY, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP1EdgeLX, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP1EdgeLY, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP1EdgeRX, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP1EdgeRY, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP0RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP1RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP0FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsStageCollisionLoopP1FloorYMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP0InitialRootXMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP1InitialRootXMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP0FinalRootXMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP1FinalRootXMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP0RootXDeltaMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP1RootXDeltaMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP0FinalRootYMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP1FinalRootYMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP0FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP1FloorYMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP0FinalDriftMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP1FinalDriftMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP0MaxDriftMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopP1MaxDriftMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorFollowLoopMaxDriftMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStageFloorEdgeLoopLeftXMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorEdgeLoopRightXMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorEdgeLoopWidthMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorEdgeLoopP0StartDistMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorEdgeLoopP1StartDistMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorEdgeLoopP0FinalDistMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorEdgeLoopP1FinalDistMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorEdgeLoopP0DeltaDistMilli, 0u),
            NDS_DIAG_WORD(gNdsStageFloorEdgeLoopP1DeltaDistMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopPrepared, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopBaseFloorEdgeSeen, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopAdapterBuildCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopAdapterCopyBackCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopAdapterFallbackLRCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopProjectFloorIDCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopProjectFloorIDHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopProjectFloorIDMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopTestNewCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopTestNewHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopTestNewMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopTestNewEdgeBranchCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopTestNewSetProjectCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopSetLandingFloorCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopSetCollideFloorCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopFCCommonPositiveDistCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopFCCommonNegativeDistCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopFCCommonZeroDistCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP0UpdateCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP1UpdateCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP0HitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP1HitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP0MissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP1MissCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsStageMPProcessFloorLoopP0FinalLineID = -1;
    gNdsStageMPProcessFloorLoopP1FinalLineID = -1;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP0FinalLineIsFloor, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP1FinalLineIsFloor, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP0FinalMaskStat, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP1FinalMaskStat, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP0FinalDistMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP1FinalDistMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP0RootYMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopP1RootYMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopInsideProbeCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopInsideProbeHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopOutsideProbeCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopOutsideProbeMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopBelowFloorProbeCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopBelowFloorPositiveDistCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopNoFinalRecenterCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopUnexpectedSceneCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopUnexpectedStatusCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPProcessFloorLoopUnsafeCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterMarioFoxStageMPUpdateFloorLoopResult = 0;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopSafeResult = 0;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopMask = 0;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopDeferredMask = 0;
    gNdsFighterMarioFoxStageMPUpdateFloorLoopCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopPrepared, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopBaseMPProcessSeen, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAdapterBuildCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAdapterCopyBackCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAdapterFallbackLRCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopUpdateMainCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopUpdateMainReturnTrueCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopUpdateMainReturnFalseCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopUpdateMainStepCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopUpdateMainMaxStepCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopUpdateMainSplitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopUpdateMainCapCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopTranslateResetCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopProcCollCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAllCollisionsCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAllCollisionsFloorHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAllCollisionsFloorMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAllCollisionsCliffEdgeBranchCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAllCollisionsStopEdgeBranchCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAllCollisionsDefaultEndCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAllCollisionsWallDeferredCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAllCollisionsCeilDeferredCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAllCollisionsFloorEdgeAdjustDeferredCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopCheckFloorCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopCheckCliffEdgeCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopCheckFloorHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopCheckCliffEdgeHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopCheckFloorMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopCheckCliffEdgeMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopInsideProbeCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopInsideProbeHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopOutsideProbeCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopOutsideProbeMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopBelowFloorProbeCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopBelowFloorHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopSplitProbeCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopSplitProbeStepCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0UpdateCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1UpdateCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0HitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1HitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0MissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1MissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0PosDiffXMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1PosDiffXMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0PosDiffYMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1PosDiffYMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0RootXBeforeMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1RootXBeforeMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0RootXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1RootXFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0RootYFinalMilli, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1RootYFinalMilli, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsStageMPUpdateFloorLoopP0FinalLineID = -1;
    gNdsStageMPUpdateFloorLoopP1FinalLineID = -1;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0FinalLineIsFloor, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1FinalLineIsFloor, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0FinalMaskStat, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1FinalMaskStat, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP0FloorOK, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopP1FloorOK, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopNoFinalRecenterCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopFallDeniedCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopOttottoDeniedCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopUnexpectedSceneCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopUnexpectedStatusCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPUpdateFloorLoopUnsafeCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsFighterMarioFoxStageMPSweepFloorLoopResult = 0;
    gNdsFighterMarioFoxStageMPSweepFloorLoopSafeResult = 0;
    gNdsFighterMarioFoxStageMPSweepFloorLoopMask = 0;
    gNdsFighterMarioFoxStageMPSweepFloorLoopDeferredMask = 0;
    gNdsFighterMarioFoxStageMPSweepFloorLoopCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopPrepared, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopBaseMPUpdateSeen, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopCheckFloorCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopCheckFloorHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopCheckFloorMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLineSweepSameCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLineSweepSameHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLineSweepSameMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLineSweepDiffCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLineSweepDiffHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLineSweepDiffMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLineSweepVisitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLineSweepCandidateCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLineSweepRejectSameLineCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLineSweepAcceptNewLineCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopSecondFloorCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopSecondFloorHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopSecondFloorMissCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopLandingFloorCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopFloorEdgeAdjustCallCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopFloorEdgeAdjustDeferredCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopMaskCurrFloorCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopMaskStatFloorEdgeClearCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopIsCollEndClearCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopSameLineProbeCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopSameLineProbeHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopDiffLineProbeCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopDiffLineProbeHitCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopNoHitProbeCount, 0u),
            NDS_DIAG_WORD(gNdsStageMPSweepFloorLoopNoHitProbeMissCount, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsRdpDefaultViewportScaleX, 0u),
            NDS_DIAG_WORD(gNdsRdpDefaultViewportScaleY, 0u),
            NDS_DIAG_WORD(gNdsRdpDefaultViewportTransX, 0u),
            NDS_DIAG_WORD(gNdsRdpDefaultViewportTransY, 0u),
            NDS_DIAG_WORD(gNdsRdpDefaultViewportScaleZ, 0u),
            NDS_DIAG_WORD(gNdsRdpDefaultViewportTransZ, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawFirstCameraNear100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawFirstCameraFar100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawFirstCameraFovY100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawFirstCameraEyeX100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawFirstCameraEyeY100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawFirstCameraEyeZ100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawFirstCameraAtX100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawFirstCameraAtY100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawFirstCameraAtZ100, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsOpeningRoomDrawMaterialBranchFirstTileUls, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawMaterialBranchFirstTileUlt, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawMaterialBranchFirstTileLrs, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawMaterialBranchFirstTileLrt, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawMaterialBranchFirstScrollUls, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawMaterialBranchFirstScrollUlt, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawMaterialBranchFirstScrollLrs, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDrawMaterialBranchFirstScrollLrt, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewTranslateX100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewTranslateY100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewTranslateZ100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewRotateX100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewRotateY100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewRotateZ100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewScaleX100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewScaleY100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewScaleZ100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewMinX, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewMaxX, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewMinY, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewMaxY, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    gNdsOpeningRoomDLPreviewProjectionMask = 0;
    gNdsOpeningRoomDLPreviewProjectionMode =
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_MODE_NONE;
    gNdsOpeningRoomDLPreviewProjectionBlocker =
        NDS_OPENING_ROOM_DL_PREVIEW_PROJECTION_BLOCKER_NONE;
    gNdsOpeningRoomDLPreviewProjectedVertexCount = 0;
    gNdsOpeningRoomDLPreviewProjectedTriangleCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewProjectedMinX, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewProjectedMaxX, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewProjectedMinY, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewProjectedMaxY, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewProjectedMinDepth100, 0u),
            NDS_DIAG_WORD(gNdsOpeningRoomDLPreviewProjectedMaxDepth100, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
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
    gNdsLBFadeCreateCount = 0;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(sNdsOpeningRoomPencilsCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomPencilsGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomPencilsDObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomPencilsXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomPencilsAObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomCloseUpOverlayCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomCloseUpOverlayGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomOutsideCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomOutsideGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomOutsideDObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomOutsideXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomHazeCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomHazeGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomHazeDObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomHazeXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomSunlightCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomSunlightGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomSunlightDObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomSunlightXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomDeskCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomDeskGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomDeskDObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomDeskXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomSpotlightCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomSpotlightGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomSpotlightDObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomSpotlightXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomSpotlightMObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomSpotlightAObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomBossShadowCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomBossShadowGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomBossShadowDObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomBossShadowXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomBossShadowAObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomScene1CameraCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomScene1CameraGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomScene1CameraCObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomScene1CameraXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomScene1CameraAObjsBefore, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
    sNdsOpeningRoomScene2EjectMainCameraGObj = NULL;
    sNdsOpeningRoomScene2EjectFighterCameraGObj = NULL;
    {
        static const uintptr_t words[] = {
            NDS_DIAG_WORD(sNdsOpeningRoomScene2CameraEjectCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomScene2CameraCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomScene2CameraGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomScene2CameraCObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomScene2CameraXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomScene2CameraAObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomCloseUpOverlayCameraCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomCloseUpOverlayCameraGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomCloseUpOverlayCameraCObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomCloseUpOverlayCameraXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomWallpaperCameraCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomWallpaperCameraGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomWallpaperCameraCObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomWallpaperCameraXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoCameraCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoCameraGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoCameraCObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoCameraXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoCameraAObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoCountsCaptured, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoGObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoDObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoXObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoMObjsBefore, 0u),
            NDS_DIAG_WORD(sNdsOpeningRoomLogoAObjsBefore, 0u),
        };
        ndsResetDiagnosticWords(words, (u32)(sizeof(words) / sizeof(words[0])));
    }
}

extern void ndsMNVSModeRunStartTransitionProbe(void);
extern void ndsMNPlayersVSRunReadyTransitionProbe(void);
extern void ndsMNMapsRunSelectVSBattleProbe(void);
extern void scVSBattleFuncUpdate(void);
extern volatile u32 gNdsFtPoseEvalTick;
