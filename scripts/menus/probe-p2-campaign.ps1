[CmdletBinding()]
param(
    [string]$Build = 'build-p2-campaign-walk',
    [string]$Target = 'smash64ds',
    [string]$Rom = '',
    [string]$Elf = '',
    [string]$BuildConfig = '',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(10, 180)][int]$TimeoutSeconds = 120,
    # Scene-entry stops, not frames: one stop is one ndsSceneManagerEnter.
    # The 1P route is Title -> ModeSelect -> 1PMode -> 1P CSS -> 1PIntro ->
    # first battle, so a handful of stops covers it; a guest the walk cannot
    # steer parks instead and the run ends by timeout (verdict BLOCKED).
    [ValidateRange(2, 64)][int]$Hits = 12,
    # Presented GO-state fight frames required before the battle screenshot.
    # Countdown/load presents do not count toward this total.
    [ValidateRange(1, 600)][int]$BattlePresents = 8,
    [string]$Artifact = '',
    [string]$Screenshot = '',
    [string]$ModeScreenshot = '',
    [string]$CssScreenshot = '',
    [string]$GoScreenshot = '',
    [string]$IntroScreenshot = '',
    [switch]$TransitionProof,
    [switch]$FullRosterHover,
    [string]$TallyScreenshot = '',
    [string]$ContinueScreenshot = ''
)

# CAMPAIGN-ENABLED ROM ACCEPTANCE PROBE. Drives a 1P build from cold boot
# over the REAL route -- native Title / ModeSelect -> source 1PMode menu ->
# source 1P CSS -> first battle -- and captures the actual scene ids, the
# battle's stage/fighter kinds and human stocks, the registry/allocator/
# asset error counters, and one presented battle frame.
#
# TWO GUEST-SIDE LEGS, both ordinary input. The native screens run on the
# scripted walk (NDS_P2_MENU_WALK / ndsMenuShellWalkTap) with the campaign
# route selected (gNdsMenuShellWalkRoute=1: Title START, then A on the opening
# 1P GAME cursor instead of the VS tour's DOWN+A). The two imported source
# menus (mn1pmode, mnplayers1pgame) read the source controller pipeline. The
# built walk reaches the first CSS entry, then this probe takes over only the
# existing DTCM controller-playback pad: B returns once to 1PMode, A re-enters,
# ordinary stick/A input changes difficulty and stock and selects Mario, a
# source R-C tap changes Mario's costume, and START commits the source menu.
# The FullRosterHover variant seeds an all-unlocked default for a fresh,
# isolated diagnostic save, then sweeps both rows twice and exits a live preview.
# Otherwise this changes controller input only. No scene_curr, battle descriptor, menu
# state, save field, fighter state or campaign state is written by GDB.
# The source menu handlers remain the only code that commits those changes.
#
# LAB BUILD CONTRACT, enforced below: NDS_P2_1P_GAME=1 (the campaign linked),
# NDS_P2_COMPACT_BATTLE_FIGHTERS=1 (battle/*.fpc residency is under test),
# NDS_P2_MENU_WALK!=0 (both walk legs compiled in), NDS_HARNESS_FAST_LOGIC=0
# (the walk's dwells and the driver's tic tables are realtime frames).
# Shipping/user ROMs keep NDS_P2_MENU_WALK=0 and human input; this probe
# refuses such a build rather than misreading it.
#
# STOP-POINT DRIVEN. Every wait is a breakpoint hit, never a wall-clock
# sleep for a menu or a match timer. (The one sleep below waits for the host
# OS to present the emulator window for the screenshot, not for the guest.)
#
# FAIL FAST. A nonzero allocator/asset/registry error counter, an ARM9 abort
# signature, or a dead emulator ends the run as FAILED, not as a verdict.

$ErrorActionPreference = 'Stop'
$scripts = Split-Path -Parent $PSScriptRoot
$root = Split-Path -Parent $scripts
. (Join-Path $scripts 'lib\melonds.ps1')
. (Join-Path $scripts 'lib\gdb-markers.ps1')
. (Join-Path $scripts 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
if ([string]::IsNullOrWhiteSpace($Rom)) {
    $Rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
}
if ([string]::IsNullOrWhiteSpace($Elf)) {
    $Elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
}
if ([string]::IsNullOrWhiteSpace($BuildConfig)) {
    $BuildConfig = Join-Path (Resolve-Smash64DSBuildPath -Root $root -Build $Build) 'nds_build_config.h'
}
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-campaign.txt')
}
if ([string]::IsNullOrWhiteSpace($Screenshot)) {
    $Screenshot = Join-Path $root ('artifacts\visibility\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-campaign-battle.png')
}
if ([string]::IsNullOrWhiteSpace($GoScreenshot)) {
    $GoScreenshot = Join-Path $root ('artifacts\\visibility\\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_1p-go.png')
}
if ([string]::IsNullOrWhiteSpace($ModeScreenshot)) {
    $ModeScreenshot = Join-Path $root ('artifacts\\\\visibility\\\\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_1p-mode.png')
}
if ([string]::IsNullOrWhiteSpace($CssScreenshot)) {
    $CssScreenshot = Join-Path $root ('artifacts\\visibility\\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_1p-css.png')
}
if ($TransitionProof -and [string]::IsNullOrWhiteSpace($TallyScreenshot)) {
    $TallyScreenshot = Join-Path $root ('artifacts\\visibility\\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_1p-tally.png')
}
if ($TransitionProof -and [string]::IsNullOrWhiteSpace($ContinueScreenshot)) {
    $ContinueScreenshot = Join-Path $root ('artifacts\\visibility\\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_1p-continue.png')
}
if (-not (Test-Path -LiteralPath $Rom -PathType Leaf)) {
    throw "p2-campaign probe: ROM not found: $Rom (pass -Rom explicitly)."
}
if (-not (Test-Path -LiteralPath $Elf -PathType Leaf)) {
    throw "p2-campaign probe: ELF not found: $Elf (pass -Elf explicitly)."
}
if (-not (Test-Path -LiteralPath $BuildConfig -PathType Leaf)) {
    throw "p2-campaign probe: $Build has no nds_build_config.h; refusing stale evidence."
}
$Rom = (Resolve-Path -LiteralPath $Rom).Path
$Elf = (Resolve-Path -LiteralPath $Elf).Path
$BuildConfig = (Resolve-Path -LiteralPath $BuildConfig).Path
$configText = Get-Content -LiteralPath $BuildConfig -Raw
# REJECT FLAG 0. A run against a ROM without the campaign linked would walk
# the VS route (or park) and read as a broken campaign rather than a wrong
# build, exactly the trap probe-p2-shell.ps1 guards for the menu shell.
foreach ($flag in @('NDS_P2_1P_GAME', 'NDS_P2_COMPACT_BATTLE_FIGHTERS')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    if ((-not $m.Success) -or ($m.Groups[1].Value -eq '0')) {
        throw "p2-campaign probe: $Build was built with $flag off; nothing to read."
    }
    Write-Output ("build config: {0}={1}" -f $flag, $m.Groups[1].Value)
}
# LAB INPUT CONTRACT. The campaign route is steered guest-side: the shell walk
# for the native screens plus the playback driver for the source menus. A ROM
# without the walk (human-input shipping config) would park on ModeSelect and
# read as a broken campaign rather than a wrong build, so refuse it up front.
# Fast logic would shrink the walk's dwells and the driver's tic tables below
# the source gates they are written against, so refuse that too.
foreach ($flag in @('NDS_P2_MENU_WALK')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    if ((-not $m.Success) -or ($m.Groups[1].Value -eq '0')) {
        throw "p2-campaign probe: $Build was built with $flag off; the campaign walk is not linked (rebuild the lab 1P config, never the shipping ROM)."
    }
    Write-Output ("build config: {0}={1}" -f $flag, $m.Groups[1].Value)
}
foreach ($flag in @('NDS_HARNESS_FAST_LOGIC')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    $value = if ($m.Success) { $m.Groups[1].Value } else { 'absent' }
    if ($value -ne '0') {
        throw "p2-campaign probe: $Build has $flag=$value; the walk needs realtime frames (need 0)."
    }
    Write-Output ("build config: {0}={1}" -f $flag, $value)
}
foreach ($flag in @('NDS_P2_MENU_SHELL',
                    'NDS_R2_SCENE_LOOP_WALK', 'NDS_DEV_SCENE_HARNESS')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    $value = if ($m.Success) { $m.Groups[1].Value } else { 'absent' }
    Write-Output ("build config: {0}={1}" -f $flag, $value)
}
$secondEntryDiagMatch = [regex]::Match(
    $configText, '(?m)^#define\s+NDS_R2_SECOND_ENTRY_DIAG\s+(\d+)u?$')
$secondEntryDiag = $secondEntryDiagMatch.Success -and
    ($secondEntryDiagMatch.Groups[1].Value -ne '0')

$required = @(
    'ndsSceneManagerEnter',
    'ndsPlatformEndFrame',
    'gSCManagerSceneData',
    'gSCManagerBattleState',
    'gNdsSceneManagerEnterCount',
    'gNdsSceneManagerRejectCount',
    'gNdsSceneManagerRingKind',
    'gNdsSceneManagerRingArenaFree',
    'gNdsSceneManagerUnregisteredEnterCount',
    'gNdsSceneManagerArenaMismatchCount',
    'gNdsSceneManagerArenaBase',
    'gNdsSceneManagerArenaSize',
    'gNdsTaskmanArenaAllocFailCount',
    'gNdsRelocAssetOpenFailCount',
    'gNdsRelocAssetFormatFailCount',
    'gNdsRelocExternalFixupFailCount',
    'gNdsRelocAssetHeaderReadCount',
    'gNdsRelocAssetPayloadReadCount',
    'gNdsRendererProfileFrameCount',
    'gNdsTaskmanGeneralHeapFreeMin',
    'gNdsPreviewPackLoadCount',
    'gNdsPreviewPackDataBytes',
    'gNdsPreviewPackFailure',
    'gNdsPreviewPackFailureKind',
    'gNdsRendererNativeFailure',
    'gNdsBattleCoreExternPatchCount',
    'gNdsBattleCoreExternLoadCount',
    'gNdsBattleCoreForeignImageBytes',
    'gNdsBattleCoreForeignImageLoadCount',
    'ndsRelocPatchCompactBattleMainExterns',
    'ftManagerSetupFilesAllKind',
    'ndsEFManagerRetryDeferredDescs',
    'gNdsSC1PGameBridgeAppliedCount',
    'gNdsSC1PGameBridgeRefusedCount',
    'gNdsBattlePlayablePacingPresentedFrames',
    'gNdsBattlePlayablePacingLogicFrames',
    'gNdsK0BattleInGo',
    'gNdsAudioBgmChunkPlayCount',
    'gNdsAudioBgmPlayCalls',
    'gNdsAudioBgmTrackID',
    'gNdsAudioFgmSupportedPlayCount',
    'gNdsAudioFgmUnsupportedCallCount',
    'sSYSchedulerTicCount',
    'gSCManager1PGameBattleState',
    'gSCManagerTransferBattleState',
    'gSCManagerVSBattleState',
    'sc1PGameSetupStageAll',
    # Campaign-walk legs: the route select the probe arms, and the controller
    # pipeline telemetry proving the driver's input reached the source menus.
    'gNdsMenuShellWalkRoute',
    'gNdsControllerPlaybackEnabled',
    'gNdsControllerPlaybackConnectedMask',
    'gNdsControllerPublishedTapMask',
    # Probe-only input control lives in DTCM in live-input-preview builds, so
    # debugger writes are coherent with the ARM9 reader.
    'sControllerPlaybackEnabled',
    'sControllerPlaybackConnectedMask',
    'sControllerPlaybackPads',
    # Imported source 1P Mode state + native presentation witnesses.
    'sMN1PModeOption',
    'gNdsOnePlayerModeNativeEnterCount',
    'gNdsOnePlayerModeNativePresentCount',
    'gNdsOnePlayerModeNativeBaseBlitCount',
    'gNdsOnePlayerModeNativeSurfaceFailCount',
    'gNdsOnePlayerModeNativeLastOption',
    'gNdsOnePlayerModeNativeLastSelected',
    'gNdsOnePlayerModeNativeVisibleMask',
    # Imported source-CSS state is read for evidence only.
    'sMNPlayers1PGameLevelValue',
    'sMNPlayers1PGameStockValue',
    'sMNPlayers1PGameSlot',
    'gNdsOnePlayerCssNativeEnterCount',
    'gNdsOnePlayerCssNativePresentCount',
    'gNdsOnePlayerCssNativeBaseBlitCount',
    'gNdsOnePlayerCssNativeSurfaceFailCount',
    'gNdsOnePlayerCssNativeLastFkind',
    'gNdsOnePlayerCssNativeLastDifficulty',
    'gNdsOnePlayerCssNativeLastStock',
    'gNdsOnePlayerCssNativeLastTime',
    'gNdsOnePlayerCssNativeVisibleMask'
)
if ($TransitionProof) {
    $required += @(
        'sc1PStageClearStartScene',
        'mnPlayers1PGameContinueStartScene',
        'ndsControllerCampaignTallyProofStop',
        'ndsControllerCampaignTallyFinalProofStop',
        'ndsControllerCampaignContinueProofStop',
        'gNdsCampaignBattlePlaybackFrameCount',
        'gNdsCampaignBattlePlaybackAttackCount',
        'gNdsCampaignBattlePlaybackApproachCount',
        'gNdsCampaignBattlePlaybackMissingOpponentCount',
        'gNdsCampaignStageClearPlaybackFrameCount',
        'gNdsCampaignStageClearPlaybackTapCount',
        'gNdsCampaignContinuePlaybackFrameCount',
        'gNdsCampaignContinuePlaybackTapCount',
        'gNdsCampaignTransitionHeapFreeMin',
        'gNdsCampaignTransitionStartStage',
        'gNdsRendererNativeFailure',
        'gNdsRendererNativeDirectReject',
        'ndsRendererRecordNativeFailure',
        'ndsSyMallocOverflowHalt',
        'sSC1PStageClearScoreTotal',
        'sSC1PStageClear1PGameStage',
        'sSC1PStageClearBonusFlags',
        'sSC1PStageClearBonusID',
        'sSC1PStageClearBonusNum',
        'sSC1PStageClearIsAllowProceedNext'
    )
    if ($secondEntryDiag) {
        $required += @(
            'gNdsAllocLedgerUsed',
            'gNdsAllocLedgerOverflow',
            'gNdsAllocLedgerTotalBytes',
            'gNdsAllocLedgerTopLR',
            'gNdsAllocLedgerTopBytes',
            'gNdsAllocLedgerTopCount'
        )
    }
}
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("p2-campaign probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
# The 1P-owned battle state is read when the 1P build links it; its absence
# is a link-stage report, not a probe defect, so it stays optional.
$has1PState = ($symbols -contains 'gSCManager1PGameBattleState')
if (-not $has1PState) {
    Write-Output 'note: gSCManager1PGameBattleState absent; CP1P will read `absent`.'
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.p2-campaign.stdout.log'
$stderr = Join-Path $log_dir 'melonds.p2-campaign.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$capture = Join-Path $scripts 'capture-running-melonds-window.ps1'
$captured = Join-Path $log_temp 'p2_campaign_probe.gdb.out'
# A failure before Invoke-GdbMarkerScript used to let finally copy the slot's
# previous transcript into this run's Artifact. Remove that stale evidence
# before melonDS starts; a failed startup must leave no transcript to publish.
Remove-Item -LiteralPath $captured -Force -ErrorAction SilentlyContinue
$config_state = $null
$emulator = $null
$timedOut = $false

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -BreakOnStartup -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    # Orchestrator timing isolation. The orchestrator creates this file while
    # an isolated timing verifier owns emulator timing. Never delete it; poll
    # at the owner-requested two-minute cadence for at most forty minutes, and
    # launch melonDS only after the file is absent.
    $orchestratorEmulatorLock = Join-Path $root 'builds\\.orchestrator-emulator.lock'
    $orchestratorEmulatorDeadline = (Get-Date).AddMinutes(40)
    while (Test-Path -LiteralPath $orchestratorEmulatorLock) {
        Write-Output 'orchestrator emulator lock present; waiting 120 seconds.'
        if ((Get-Date) -ge $orchestratorEmulatorDeadline) {
            throw 'orchestrator emulator lock remained present for 40 minutes; refusing slot-7 launch.'
        }
        Start-Sleep -Seconds 120
    }
    # Hidden, like every other launch in this tree. The comment this replaces
    # claimed a hidden launch leaves MainWindowHandle at IntPtr.Zero and so
    # photographs black; that is not what happens --
    # builds/resume-20260908/stage-witness-probe.ps1 launches hidden and its
    # captures (artifacts/visibility/2026-09-08_witness-*.png) show the running
    # stage. An unhidden launch steals the owner's foreground and
    # check-melonds-policy.ps1 fails the whole tree for it.
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -WindowStyle Hidden `
        -PassThru
    # A hidden Qt window is not guaranteed to populate Process.MainWindowHandle
    # even while it is healthy. capture-running-melonds-window.ps1 already owns
    # the robust PID -> EnumWindows lookup for evidence. Here the GDB listener
    # is the readiness signal; fail only if the process itself exited.
    Start-Sleep -Milliseconds 250
    $emulator.Refresh()
    if ($emulator.HasExited) {
        throw 'melonDS exited before the GDB listener became ready.'
    }
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    # Scene ids are the SCKind enum (include/sc/scene.h): Title 1,
    # ModeSelect 7, 1PMode 8, 1PGamePlayers 17, 1PGame 52. Fighter kinds are
    # FTKind (include/ft/fighter.h: Mario 0 .. Link 5); grounds are GRKind
    # (scene.h: Castle 0 .. Hyrule 4). Printed raw so the artifact, not this
    # script, is the record main validates.
    $battleLines = @(
        'printf "CPBATTLE %d gametype=%d gkind=%02x time=%u pl=%u cp=%u s0=%u/%u/%u s1=%u/%u/%u s2=%u/%u/%u s3=%u/%u/%u\n", $n, gSCManagerBattleState->game_type, gSCManagerBattleState->gkind, gSCManagerBattleState->time_limit, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count, gSCManagerBattleState->players[0].fkind, gSCManagerBattleState->players[0].pkind, gSCManagerBattleState->players[0].stock_count, gSCManagerBattleState->players[1].fkind, gSCManagerBattleState->players[1].pkind, gSCManagerBattleState->players[1].stock_count, gSCManagerBattleState->players[2].fkind, gSCManagerBattleState->players[2].pkind, gSCManagerBattleState->players[2].stock_count, gSCManagerBattleState->players[3].fkind, gSCManagerBattleState->players[3].pkind, gSCManagerBattleState->players[3].stock_count'
    )
    $stopLines = @(
        'printf "CPLINE %d curr=%u prev=%u enters=%u rej=%u unreg=%u mism=%u arenabase=%08x arenasize=%u\n", $n, gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsSceneManagerEnterCount, gNdsSceneManagerRejectCount, gNdsSceneManagerUnregisteredEnterCount, gNdsSceneManagerArenaMismatchCount, gNdsSceneManagerArenaBase, gNdsSceneManagerArenaSize',
        'printf "CPERR %d allocfail=%u openfail=%u formatfail=%u fixupfail=%u hdr=%u payload=%u\n", $n, gNdsTaskmanArenaAllocFailCount, gNdsRelocAssetOpenFailCount, gNdsRelocAssetFormatFailCount, gNdsRelocExternalFixupFailCount, gNdsRelocAssetHeaderReadCount, gNdsRelocAssetPayloadReadCount',
        'printf "CPGFX %d peak=%u capacity=%u overflow=%u noroom=%u dl_overflow=%u dl_kind=%u dl_bytes=%u\n", $n, gNdsTaskmanGraphicsHeapHighWater, gNdsTaskmanGraphicsHeapCapacity, gNdsTaskmanGraphicsHeapOverflowCount, gNdsTaskmanGraphicsHeapNoRoomCount, gNdsTaskmanDLOverflowCount, gNdsTaskmanDLOverflowKind, gNdsTaskmanDLOverflowBytes',
        # Controller-pipeline proof: the driver's A/START taps must publish
        # through the source edge accumulator (bit 15 set == A delivered).
        'printf "CPCTL %d en=%u mask=%x published=%x\n", $n, gNdsControllerPlaybackEnabled, gNdsControllerPlaybackConnectedMask, gNdsControllerPublishedTapMask',
        'if gSCManagerBattleState != 0'
    ) + $battleLines + @(
        'else',
        'printf "CPBATTLE %d none\n", $n',
        'end',
        $(if ($has1PState) {
            'printf "CP1P %d player=%u fkind=%u costume=%u diff=%u stocks=%u stage=%u\n", $n, gSCManagerSceneData.player, gSCManagerSceneData.fkind, gSCManagerSceneData.costume, gSCManagerBackupData.spgame_difficulty, gSCManagerBackupData.spgame_stock_count, gSCManagerSceneData.spgame_stage'
        } else {
            'printf "CP1P %d absent\n", $n'
        })
    )
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set print elements 128',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $n = 0',
        'set $inbattle = 0',
        'set $battleframes = 0',
        'set $goframes = 0',
        'set $saw_go = 0',
        'set $saw_css_a = 0',
        'set $introshot = 0',
        'set $introaudio = 0',
        'set $inintro = 0',
        'set $intro_bgm_calls = 0',
        'set $intro_bgm_id = -1',
        'set $intro_fgm_calls = 0',
        'set $intro_fgm_last = -1',
        'set $intro_bgm_base = 0',
        'set $intro_bgm_play_base = 0',
        'set $intro_fgm_base = 0',
        'set $intro_fgm_unsupported_base = 0',
        'set $manual = 0',
        'set $cssvisits = 0',
        'set $css_tick = 0',
        'set $css_native_base = 0',
        'set $mode_tick = 0',
        'set $mode_shot = 0',
        'set $mode_native_base = 0',
        'set $backdone = 0',
        'set $base_diff = 0',
        'set $base_stock = 0',
        'set $base_costume = 0',
        'set $use_right = 1',
        'set $css_mut_printed = 0',
        'set $setupcount = 0',
        'set $startup_free = 0',
        'set $startup_guest_min = 0',
        'set $active_min = 0xffffffff',
        'set $battle_arena = 0',
        'set $fpc_begin_free = 0',
        'set $fpc_kind = -1',
        'set $ring_free_printed = 0',
        'set $extern_before = 0',
        'set $extern_fighters = 0',
        'set $intro_enters = 0',
        'set $intro_rejects = 0',
        'set $intro_link_seen = 0',
        'set $intro_link_before = 0xffffffff',
        'set $intro_link_after = 0xffffffff',
        $(if ($TransitionProof) { 'set $transition = 1' } else { 'set $transition = 0' }),
        'set $first_stage = -1',
        'set $saw_stageclear = 0',
        'set $saw_continue = 0',
        'set $native_win = 0',
        'set $stageclear_frames = 0',
        'set $continue_frames = 0',
        'set $tallyshot = 0',
        'set $tallyfinal = 0',
        'set $intro_native_printed = 0',
        'set $tally_native_printed = 0',
        'set $intro_reject_printed = 0',
        'set $tally_reject_printed = 0',
        $(if ($FullRosterHover) {
            'set $hover_mask = 0'
            'set $hover_draw1 = 0'
            'set $hover_draw2 = 0'
            'set $hover_min = 0xffffffff'
            'set $hover_last = -2'
            'set $hover_done = 0'
            # Seed the fresh diagnostic save before the default backup is first
            # read. No live cached menu state or production save is changed.
            'tbreak lbBackupIsSramValid'
            'commands'
            'silent'
            'set $maskword = (unsigned int *)(((unsigned int)&dSCManagerDefaultBackupData.fighter_mask) & ~3)'
            'set $maskshift = (((unsigned int)&dSCManagerDefaultBackupData.fighter_mask) & 3) * 8'
            'set variable *$maskword = (*$maskword & ~(65535 << $maskshift)) | (4095 << $maskshift)'
            'printf "CPHOVER_FIXTURE all_unlocked=1\n"'
            'continue'
            'end'
        }),
        'break ndsSceneManagerEnter',
        'commands',
        'silent',
        # Set after runtime/BSS initialization and before the native menu
        # processes input. A boot-time write could be cleared by startup.
        'if $manual == 0',
        'set variable gNdsMenuShellWalkRoute = 1',
        'end',
        'set $n = $n + 1'
    ) + $stopLines + @(
        'if gSCManagerSceneData.scene_curr == 8',
        'set $mode_native_base = gNdsRendererNativeFailure.count',
        $(if ($FullRosterHover) {
            'if $hover_done != 0'
            'printf "CPHOVER_EXIT resident_kind=%d main=%#x model=%#x\n", sNdsPlayers1PGamePreviewFkind, *dFTManagerDataFiles[11]->p_file_main, *dFTManagerDataFiles[11]->p_file_model'
            'detach'
            'quit'
            'end'
        }),
        'end',
        'if gSCManagerSceneData.scene_curr == 17',
        'set $css_native_base = gNdsRendererNativeFailure.count',
        'set $cssvisits = $cssvisits + 1',
        'set $css_tick = 0',
        'if $cssvisits == 1',
        # The first source-CSS entry is reached by the built guest walk. From
        # here on, keep that walk out and drive only its DTCM playback pad.
        'set $manual = 1',
        'set variable gNdsMenuShellWalkRoute = 0',
        'set variable sControllerPlaybackEnabled = 1',
        'set variable sControllerPlaybackConnectedMask = 1',
        'set variable sControllerPlaybackPads[0].button = 0',
        'set variable sControllerPlaybackPads[0].stick_x = 0',
        'set variable sControllerPlaybackPads[0].stick_y = 0',
        'printf "CPCSSCTRL manual=1 visit=%d\n", $cssvisits',
        'end',
        'end',
        'if (gSCManagerSceneData.scene_curr == 8) && ($manual != 0)',
        'set $mode_tick = 0',
        'end',
        'if (gSCManagerSceneData.scene_curr == 14) && ($manual != 0)',
        'set $inintro = 1',
        'set $intro_bgm_base = gNdsAudioBgmChunkPlayCount',
        'set $intro_bgm_play_base = gNdsAudioBgmPlayCalls',
        'set $intro_fgm_base = gNdsAudioFgmSupportedPlayCount',
        'set $intro_fgm_unsupported_base = gNdsAudioFgmUnsupportedCallCount',
        'end',
        # nSCKind1PGame (52) is the first campaign battle. Its own entry stop
        # is pre-presentation, so step presents, photograph the halted window
        # (a halted melonDS keeps its last frame up), and leave.
        'if gSCManagerSceneData.scene_curr == 52',
        'set $inintro = 0',
        'printf "CPBATTLE-HIT %d renderframe=%u\n", $n, gNdsRendererProfileFrameCount',
        'set $inbattle = 1',
        'end',
        'if $transition != 0',
        'if gSCManagerSceneData.scene_curr == 52',
        'if $first_stage < 0',
        'set $first_stage = gSCManagerSceneData.spgame_stage',
        'set $native_entry = gNdsRendererNativeFailure.count',
        'printf "CPTRANSITION-FIRST stage=%d enters=%u\n", $first_stage, gNdsSceneManagerEnterCount',
        'eval "disable %d", $frame_bp',
        'printf "CPFRAMEBP-DISABLED bp=%d\n", $frame_bp',
        'else',
        'if ($saw_stageclear != 0) && (gSCManagerSceneData.spgame_stage > $first_stage)',
        'printf "CPNEXTBATTLE stage=%u first_stage=%d enters=%u score=%d bonuses=%u player_stock=%d heap_min=%u native_fail=%u native_transition_delta=%u drive_frames=%u attacks=%u approaches=%u missing_opp=%u\n", gSCManagerSceneData.spgame_stage, $first_stage, gNdsSceneManagerEnterCount, gSCManagerSceneData.spgame_score, gSCManagerSceneData.bonus_count, gSCManager1PGameBattleState.players[gSCManagerSceneData.player].stock_count, gNdsCampaignTransitionHeapFreeMin, gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.count-$native_win, gNdsCampaignBattlePlaybackFrameCount, gNdsCampaignBattlePlaybackAttackCount, gNdsCampaignBattlePlaybackApproachCount, gNdsCampaignBattlePlaybackMissingOpponentCount',
        'printf "CPTRANSITIONDONE n=%d stageclear=%d continue=%d\n", $n, $saw_stageclear, $saw_continue',
        'detach',
        'quit',
        'end',
        'end',
        'end',
        'if gSCManagerSceneData.scene_curr == 51',
        'set $inbattle = 0',
        'set $saw_stageclear = 1',
        'set $stageclear_frames = 0',
        'set $tallyshot = 0',
        'set $tallyfinal = 0',
        'printf "CPSTAGECLEAR-ENTER prev=%u stage=%u score=%d masks=%08x/%08x/%08x\n", gSCManagerSceneData.scene_prev, gSCManagerSceneData.spgame_stage, gSCManagerSceneData.spgame_score, gSCManagerSceneData.bonus_get_mask[0], gSCManagerSceneData.bonus_get_mask[1], gSCManagerSceneData.bonus_get_mask[2]',
        'end',
        'if gSCManagerSceneData.scene_curr == 49',
        'set $inbattle = 0',
        'set $saw_continue = 1',
        'set $continue_frames = 0',
        'printf "CPCONTINUE-ENTER prev=%u stage=%u player_stock=%d time=%d\n", gSCManagerSceneData.scene_prev, gSCManagerSceneData.spgame_stage, gSCManager1PGameBattleState.players[gSCManagerSceneData.player].stock_count, gSCManager1PGameBattleState.time_remain',
        'end',
        'if (gSCManagerSceneData.scene_curr == 14) && ($saw_stageclear != 0)',
        'printf "CPNEXTINTRO stage=%u first_stage=%d score=%d\n", gSCManagerSceneData.spgame_stage, $first_stage, gSCManagerSceneData.spgame_score',
        'end',
        'end',
        ('if $n < ' + $Hits),
        'continue',
        'end',
        'end',
        'break sc1PGameSetupStageAll',
        'commands',
        'silent',
        'set $setupcount = $setupcount + 1',
        'printf "CPSETUP count=%d stage=%u state=%08x\n", $setupcount, gSCManagerSceneData.spgame_stage, gSCManagerBattleState',
        'continue',
        'end',
        $(if ($TransitionProof) { 'break *ndsRendererRecordNativeFailure' }),
        $(if ($TransitionProof) { 'commands' }),
        $(if ($TransitionProof) { 'silent' }),
        $(if ($TransitionProof) { 'if ($r1 == 14) && ($intro_native_printed == 0)' }),
        $(if ($TransitionProof) { 'set $intro_native_printed = 1' }),
        $(if ($TransitionProof) { 'printf "CPNATIVE-INTRO domain=%u scene=%u identity=%u status=%u root=%u material=%u reason=%u reject_count=%u reject_site=%08x\n", $r0, $r1, $r2, $r3, *(unsigned*)$sp, *(unsigned*)($sp+4), *(unsigned*)($sp+8), gNdsRendererNativeDirectReject.count, gNdsRendererNativeDirectReject.site' }),
        $(if ($TransitionProof) { 'end' }),
        $(if ($TransitionProof) { 'if ($r1 == 51) && ($tally_native_printed == 0)' }),
        $(if ($TransitionProof) { 'set $tally_native_printed = 1' }),
        $(if ($TransitionProof) { 'printf "CPNATIVE-TALLY domain=%u scene=%u identity=%u status=%u root=%u material=%u reason=%u reject_count=%u reject_site=%08x\n", $r0, $r1, $r2, $r3, *(unsigned*)$sp, *(unsigned*)($sp+4), *(unsigned*)($sp+8), gNdsRendererNativeDirectReject.count, gNdsRendererNativeDirectReject.site' }),
        $(if ($TransitionProof) { 'end' }),
        $(if ($TransitionProof) { 'continue' }),
        $(if ($TransitionProof) { 'end' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'break ndsSyMallocOverflowHalt' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'commands' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'silent' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'printf "CPOVERFLOW request=%u free=%u align=%u caller=%08x ledger_used=%u ledger_total=%u ledger_overflow=%u\n", gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom, gNdsSyMallocOverflowAlignment, gNdsSyMallocOverflowCallerLR, gNdsAllocLedgerUsed, gNdsAllocLedgerTotalBytes, gNdsAllocLedgerOverflow' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'printf "CPOWNER rank=0 lr=%08x bytes=%u count=%u\n", gNdsAllocLedgerTopLR[0], gNdsAllocLedgerTopBytes[0], gNdsAllocLedgerTopCount[0]' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'printf "CPOWNER rank=1 lr=%08x bytes=%u count=%u\n", gNdsAllocLedgerTopLR[1], gNdsAllocLedgerTopBytes[1], gNdsAllocLedgerTopCount[1]' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'printf "CPOWNER rank=2 lr=%08x bytes=%u count=%u\n", gNdsAllocLedgerTopLR[2], gNdsAllocLedgerTopBytes[2], gNdsAllocLedgerTopCount[2]' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'printf "CPOWNER rank=3 lr=%08x bytes=%u count=%u\n", gNdsAllocLedgerTopLR[3], gNdsAllocLedgerTopBytes[3], gNdsAllocLedgerTopCount[3]' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'printf "CPOWNER rank=4 lr=%08x bytes=%u count=%u\n", gNdsAllocLedgerTopLR[4], gNdsAllocLedgerTopBytes[4], gNdsAllocLedgerTopCount[4]' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'printf "CPOWNER rank=5 lr=%08x bytes=%u count=%u\n", gNdsAllocLedgerTopLR[5], gNdsAllocLedgerTopBytes[5], gNdsAllocLedgerTopCount[5]' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'printf "CPOWNER rank=6 lr=%08x bytes=%u count=%u\n", gNdsAllocLedgerTopLR[6], gNdsAllocLedgerTopBytes[6], gNdsAllocLedgerTopCount[6]' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'printf "CPOWNER rank=7 lr=%08x bytes=%u count=%u\n", gNdsAllocLedgerTopLR[7], gNdsAllocLedgerTopBytes[7], gNdsAllocLedgerTopCount[7]' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'detach' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'quit' }),
        $(if ($TransitionProof -and $secondEntryDiag) { 'end' }),
        $(if ($TransitionProof) { 'break sc1PStageClearStartScene' }),
        $(if ($TransitionProof) { 'commands' }),
        $(if ($TransitionProof) { 'silent' }),
        $(if ($TransitionProof) { 'set $native_win = gNdsRendererNativeFailure.count' }),
        $(if ($TransitionProof) { 'printf "CPWINROUTE stage=%u p0=%d/%d p1=%d/%d p2=%d/%d p3=%d/%d time=%d score=%d masks=%08x/%08x/%08x heap_min=%u native_entry=%u native_fail=%u native_battle_delta=%u drive_frames=%u attacks=%u\n", gSCManagerSceneData.spgame_stage, gSCManager1PGameBattleState.players[0].pkind, gSCManager1PGameBattleState.players[0].stock_count, gSCManager1PGameBattleState.players[1].pkind, gSCManager1PGameBattleState.players[1].stock_count, gSCManager1PGameBattleState.players[2].pkind, gSCManager1PGameBattleState.players[2].stock_count, gSCManager1PGameBattleState.players[3].pkind, gSCManager1PGameBattleState.players[3].stock_count, gSCManager1PGameBattleState.time_remain, gSCManagerSceneData.spgame_score, gSCManagerSceneData.bonus_get_mask[0], gSCManagerSceneData.bonus_get_mask[1], gSCManagerSceneData.bonus_get_mask[2], gNdsCampaignTransitionHeapFreeMin, $native_entry, gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.count-$native_entry, gNdsCampaignBattlePlaybackFrameCount, gNdsCampaignBattlePlaybackAttackCount' }),
        $(if ($TransitionProof) { 'continue' }),
        $(if ($TransitionProof) { 'end' }),
        $(if ($TransitionProof) { 'break mnPlayers1PGameContinueStartScene' }),
        $(if ($TransitionProof) { 'commands' }),
        $(if ($TransitionProof) { 'silent' }),
        $(if ($TransitionProof) { 'printf "CPLOSSROUTE stage=%u player_stock=%d time=%d continues=%u heap_min=%u native_fail=%u drive_frames=%u attacks=%u\n", gSCManagerSceneData.spgame_stage, gSCManager1PGameBattleState.players[gSCManagerSceneData.player].stock_count, gSCManager1PGameBattleState.time_remain, gSCManagerSceneData.continues_used, gNdsCampaignTransitionHeapFreeMin, gNdsRendererNativeFailure.count, gNdsCampaignBattlePlaybackFrameCount, gNdsCampaignBattlePlaybackAttackCount' }),
        $(if ($TransitionProof) { 'continue' }),
        $(if ($TransitionProof) { 'end' }),
        $(if ($TransitionProof) { 'break ndsControllerCampaignTallyProofStop' }),
        $(if ($TransitionProof) { 'commands' }),
        $(if ($TransitionProof) { 'silent' }),
        $(if ($TransitionProof) { 'printf "CPTALLY-SHOT frame=%u stage=%d score=%d bonus_num=%d bonus_id=%d flags=%08x/%08x/%08x allow=%d player_stock=%d heap_min=%u native_fail=%u taps=%u\n", gNdsCampaignStageClearPlaybackFrameCount, sSC1PStageClear1PGameStage, sSC1PStageClearScoreTotal, sSC1PStageClearBonusNum, sSC1PStageClearBonusID, sSC1PStageClearBonusFlags[0], sSC1PStageClearBonusFlags[1], sSC1PStageClearBonusFlags[2], sSC1PStageClearIsAllowProceedNext, gSCManager1PGameBattleState.players[gSCManagerSceneData.player].stock_count, gNdsCampaignTransitionHeapFreeMin, gNdsRendererNativeFailure.count, gNdsCampaignStageClearPlaybackTapCount' }),
        $(if ($TransitionProof) { ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture + '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $TallyScreenshot + '"') }),
        $(if ($TransitionProof) { 'continue' }),
        $(if ($TransitionProof) { 'end' }),
        $(if ($TransitionProof) { 'break ndsControllerCampaignTallyFinalProofStop' }),
        $(if ($TransitionProof) { 'commands' }),
        $(if ($TransitionProof) { 'silent' }),
        $(if ($TransitionProof) { 'printf "CPTALLY-FINAL frame=%u stage=%d score=%d bonus_num=%d bonus_id=%d flags=%08x/%08x/%08x allow=%d player_stock=%d heap_min=%u native_fail=%u taps=%u\n", gNdsCampaignStageClearPlaybackFrameCount, sSC1PStageClear1PGameStage, sSC1PStageClearScoreTotal, sSC1PStageClearBonusNum, sSC1PStageClearBonusID, sSC1PStageClearBonusFlags[0], sSC1PStageClearBonusFlags[1], sSC1PStageClearBonusFlags[2], sSC1PStageClearIsAllowProceedNext, gSCManager1PGameBattleState.players[gSCManagerSceneData.player].stock_count, gNdsCampaignTransitionHeapFreeMin, gNdsRendererNativeFailure.count, gNdsCampaignStageClearPlaybackTapCount' }),
        $(if ($TransitionProof) { 'continue' }),
        $(if ($TransitionProof) { 'end' }),
        $(if ($TransitionProof) { 'break ndsControllerCampaignContinueProofStop' }),
        $(if ($TransitionProof) { 'commands' }),
        $(if ($TransitionProof) { 'silent' }),
        $(if ($TransitionProof) { 'printf "CPCONTINUE-SHOT frame=%u stage=%u player_stock=%d time=%d heap_min=%u native_fail=%u taps=%u\n", gNdsCampaignContinuePlaybackFrameCount, gSCManagerSceneData.spgame_stage, gSCManager1PGameBattleState.players[gSCManagerSceneData.player].stock_count, gSCManager1PGameBattleState.time_remain, gNdsCampaignTransitionHeapFreeMin, gNdsRendererNativeFailure.count, gNdsCampaignContinuePlaybackTapCount' }),
        $(if ($TransitionProof) { ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture + '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $ContinueScreenshot + '"') }),
        $(if ($TransitionProof) { 'continue' }),
        $(if ($TransitionProof) { 'end' }),
        # Measure the first Intro stage's Link tree around the source fighter
        # setup call. The FPC marker makes the accepted full-file Intro
        # residency (battle=0) distinct from compact battle ownership.
        'break ftManagerSetupFilesAllKind',
        'commands',
        'silent',
        'if (gNdsSceneManagerCurrKind == 14) && ($r0 == 5) && ($intro_link_seen == 0)',
        'set $intro_link_seen = 1',
        'set $intro_link_before = (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr',
        'printf "CPINTRO-LINK-BEFORE free=%u\n", $intro_link_before',
        'end',
        'continue',
        'end',
        'break ndsEFManagerRetryDeferredDescs',
        'commands',
        'silent',
        'if (gNdsSceneManagerCurrKind == 14) && ($intro_link_seen != 0) && ($intro_link_after == 0xffffffff)',
        'set $intro_link_after = (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr',
        'printf "CPINTRO-LINK-AFTER free=%u paid=%u\n", $intro_link_after, $intro_link_before-$intro_link_after',
        'end',
        'continue',
        'end',
        'break ndsRelocLoadPreviewFighterUnlocked',
        'commands',
        'silent',
        'if (gNdsSceneManagerCurrKind == 14) || (gNdsSceneManagerCurrKind == 52)',
        'set $fpc_kind = $r0',
        'set $fpc_begin_free = (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr',
        'printf "CPFPCBEGIN scene=%u battle=%u kind=%d free=%u\n", gNdsSceneManagerCurrKind, gNdsSceneManagerCurrIsBattle, $fpc_kind, $fpc_begin_free',
        'if (gNdsSceneManagerCurrKind == 52) && ($ring_free_printed == 0) && (gNdsSceneManagerEnterCount >= 2)',
        'set $ring_free_printed = 1',
        'set $prev_ring = (gNdsSceneManagerEnterCount - 2) % 16',
        'printf "CPRINGFREE prev_kind=%u prev_free=%u enters=%u current_free=%u\n", gNdsSceneManagerRingKind[$prev_ring], gNdsSceneManagerRingArenaFree[$prev_ring], gNdsSceneManagerEnterCount, $fpc_begin_free',
        'end',
        'printf "CPALLOC size=%u free=%u lr=%08x\n", $r0, (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr, $lr',
        'end',
        'continue',
        'end',
        # Runtime residency witness. At this function entry the just-loaded
        # fighter's FPC allocation has already been charged to DataBytes, while
        # its extern closure has not yet been patched. Consecutive Mario/Link
        # entries therefore expose Link's actual compact FPC delta directly.
        'break ndsRelocPatchCompactBattleMainExterns',
        'commands',
        'silent',
        'if (gNdsSceneManagerCurrKind == 14) || (gNdsSceneManagerCurrKind == 52)',
        'set $extern_before = gNdsBattleCoreExternPatchCount',
        'printf "CPPACKKIND scene=%u kind=%d free=%u loads=%u bytes=%u extern_patches=%u extern_loads=%u foreign_bytes=%u\n", gNdsSceneManagerCurrKind, $r0, (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr, gNdsPreviewPackLoadCount, gNdsPreviewPackDataBytes, gNdsBattleCoreExternPatchCount, gNdsBattleCoreExternLoadCount, gNdsBattleCoreForeignImageBytes',
        'end',
        'continue',
        'end',
        'break ndsFTManagerSetupCompactBattleFilesKind',
        'commands',
        'silent',
        'if gNdsSceneManagerCurrKind == 52',
        'set $extern_fighters = $extern_fighters + 1',
        'printf "CPEXTERNAUDIT kind=%d free=%u extern_before=%u extern_after=%u extern_delta=%u\n", $fpc_kind, (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr, $extern_before, gNdsBattleCoreExternPatchCount, gNdsBattleCoreExternPatchCount-$extern_before',
        'printf "CPEXTERNDONE kind=%d free=%u extern_patches=%u extern_loads=%u foreign_bytes=%u\n", $r0, (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr, gNdsBattleCoreExternPatchCount, gNdsBattleCoreExternLoadCount, gNdsBattleCoreForeignImageBytes',
        'end',
        'continue',
        'end',
        # Call-site witnesses avoid the known stale-cache problem of debugger
        # reads from ordinary ARM9 globals. Arguments are live in r0/r1 here.
        'break syAudioPlayBGM',
        'commands',
        'silent',
        'if $inintro != 0',
        'set $intro_bgm_calls = $intro_bgm_calls + 1',
        'set $intro_bgm_id = $r1',
        'printf "CPINTRO-BGM call=%d id=%d\n", $intro_bgm_calls, $intro_bgm_id',
        'end',
        'continue',
        'end',
        'break func_800269C0_275C0',
        'commands',
        'silent',
        'if $inintro != 0',
        'set $intro_fgm_calls = $intro_fgm_calls + 1',
        'set $intro_fgm_last = $r0',
        'printf "CPINTRO-FGM call=%d id=%d\n", $intro_fgm_calls, $intro_fgm_last',
        'end',
        'continue',
        'end',
        # GDB discards the rest of a breakpoint command list on continue.
        # Count frame hits in their own list; never put a capture after a
        # continue expecting the previous list to resume.
        'break ndsPlatformEndFrame',
        'set $frame_bp = $bpnum',
        'commands',
        'silent',
        # The first source 1P Mode visit is driven by the built walk and lasts
        # twelve source tics before A. Capture after several native presents,
        # while the source still owns option 0 in ordinary Highlight state.
        'if (gNdsSceneManagerCurrKind == 8) && ($mode_shot == 0) && (gNdsOnePlayerModeNativePresentCount >= 4)',
        'set $mode_shot = 1',
        'printf "CPMODEVIS source=%u enter=%u present=%u base=%u fail=%u mask=%x option=%u selected=%u native=%u native_delta=%u\n", sMN1PModeOption, gNdsOnePlayerModeNativeEnterCount, gNdsOnePlayerModeNativePresentCount, gNdsOnePlayerModeNativeBaseBlitCount, gNdsOnePlayerModeNativeSurfaceFailCount, gNdsOnePlayerModeNativeVisibleMask, gNdsOnePlayerModeNativeLastOption, gNdsOnePlayerModeNativeLastSelected, gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.count-$mode_native_base',
        ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture +
         '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $ModeScreenshot + '"'),
        'end',
        # After the built walk reaches source 1P CSS, drive only the existing
        # DTCM playback pad. Source menu code still performs every state change.
        'if $manual != 0',
        'set variable sControllerPlaybackPads[0].button = 0',
        'set variable sControllerPlaybackPads[0].stick_x = 0',
        'set variable sControllerPlaybackPads[0].stick_y = 0',
        'if gNdsSceneManagerCurrKind == 17',
        'set $css_tick = $css_tick + 1',
        $(if ($FullRosterHover) {
            'if $css_tick > 2'
            'set $hover_free = (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr'
            'if $hover_free < $hover_min'
            'set $hover_min = $hover_free'
            'end'
            'if (sMNPlayers1PGameSlot.fkind >= 0) && (sMNPlayers1PGameSlot.fkind <= 11) && (sMNPlayers1PGameSlot.player != 0)'
            'set $hover_mask = $hover_mask | (1 << sMNPlayers1PGameSlot.fkind)'
            'if ($hover_last == sMNPlayers1PGameSlot.fkind) && (gNdsFighterDLAllDrawP0HardwareTriangleCount > 0)'
            'if ($css_tick >= 130) && ($css_tick <= 294)'
            'set $hover_draw1 = $hover_draw1 | (1 << $hover_last)'
            'end'
            'if ($css_tick >= 310) && ($css_tick <= 484)'
            'set $hover_draw2 = $hover_draw2 | (1 << $hover_last)'
            'end'
            'end'
            'end'
            'if sMNPlayers1PGameSlot.fkind != $hover_last'
            'set $hover_last = sMNPlayers1PGameSlot.fkind'
            'printf "CPHOVERK tick=%d kind=%d triangles=%u free=%u\n", $css_tick, $hover_last, gNdsFighterDLAllDrawP0HardwareTriangleCount, $hover_free'
            'end'
            'end'
        }),
        'if $cssvisits == 1',
        'if $css_tick == 2',
        'set $base_diff = sMNPlayers1PGameLevelValue',
        'set $base_stock = sMNPlayers1PGameStockValue',
        'set $base_costume = sMNPlayers1PGameSlot.costume',
        'set $use_right = (($base_diff < 4) && ($base_stock < 4))',
        $(if ($FullRosterHover) {
            'if ($use_right == 0) || (sMNPlayers1PGameFighterMask != 4095)'
            'printf "CPHOVER_INVALID_FIXTURE fresh_default_save_required=1\n"'
            'quit 2'
            'end'
        }),
        'printf "CPCSSBASE visit=1 diff=%u stock=%u fkind=%d costume=%u use_right=%u\n", $base_diff, $base_stock, sMNPlayers1PGameSlot.fkind, $base_costume, $use_right',
        'end',
        # One ordinary B cancellation after the source 10-tic entry gate.
        'if ($css_tick == 12) || ($css_tick == 13)',
        'set variable sControllerPlaybackPads[0].button = 0x4000',
        'set $backdone = 1',
        'end',
        'else',
        # Second visit: change difficulty and stock on one arrow column, then
        # move the grabbed puck to Mario (portrait 1), select, change costume
        # with source R-C, and START after the source 60-tic gate.
        'if $css_tick == 2',
        'printf "CPCSSBASE visit=2 diff=%u stock=%u fkind=%d costume=%u back=%u\n", sMNPlayers1PGameLevelValue, sMNPlayers1PGameStockValue, sMNPlayers1PGameSlot.fkind, sMNPlayers1PGameSlot.costume, $backdone',
        'end',
        'if $use_right != 0',
        'if ($css_tick >= 5) && ($css_tick <= 49)',
        'set variable sControllerPlaybackPads[0].stick_x = 80',
        'end',
        'if ($css_tick == 51) || ($css_tick == 52)',
        'set variable sControllerPlaybackPads[0].button = 0x8000',
        'end',
        'if ($css_tick >= 55) && ($css_tick <= 58)',
        'set variable sControllerPlaybackPads[0].stick_y = -80',
        'end',
        'if ($css_tick == 60) || ($css_tick == 61)',
        'set variable sControllerPlaybackPads[0].button = 0x8000',
        'end',
        'if ($css_tick >= 65) && ($css_tick <= 96)',
        'set variable sControllerPlaybackPads[0].stick_x = -80',
        'set variable sControllerPlaybackPads[0].stick_y = 80',
        'end',
        'if ($css_tick >= 97) && ($css_tick <= 109)',
        'set variable sControllerPlaybackPads[0].stick_x = -80',
        'end',
        $(if ($FullRosterHover) {
            'if ($css_tick >= 112) && ($css_tick <= 128)'
            'set variable sControllerPlaybackPads[0].stick_x = -80'
            'end'
            'if ($css_tick >= 130) && ($css_tick <= 204)'
            'set variable sControllerPlaybackPads[0].stick_x = 80'
            'end'
            'if ($css_tick >= 207) && ($css_tick <= 217)'
            'set variable sControllerPlaybackPads[0].stick_y = -80'
            'end'
            'if ($css_tick >= 220) && ($css_tick <= 294)'
            'set variable sControllerPlaybackPads[0].stick_x = -80'
            'end'
            'if ($css_tick >= 297) && ($css_tick <= 307)'
            'set variable sControllerPlaybackPads[0].stick_y = 80'
            'end'
            'if ($css_tick >= 310) && ($css_tick <= 384)'
            'set variable sControllerPlaybackPads[0].stick_x = 80'
            'end'
            'if ($css_tick >= 387) && ($css_tick <= 397)'
            'set variable sControllerPlaybackPads[0].stick_y = -80'
            'end'
            'if ($css_tick >= 400) && ($css_tick <= 466)'
            'set variable sControllerPlaybackPads[0].stick_x = -80'
            'end'
            'if $css_tick == 484'
            'printf "CPHOVER mask=%x loads=%u bytes=%u failure=%u kind=%u overflow=%u free=%u min=%u native_delta=%u\n", $hover_mask, gNdsPreviewPackLoadCount, gNdsPreviewPackDataBytes, gNdsPreviewPackFailure, gNdsPreviewPackFailureKind, gNdsSyMallocOverflowCount, (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr, $hover_min, gNdsRendererNativeFailure.count-$css_native_base'
            'printf "CPHOVER_DRAW lap1=%x lap2=%x rejects=%u arena=%u\n", $hover_draw1, $hover_draw2, gNdsFtrRejectCountBySlot[0], gNdsTaskmanArenaChosenSize'
            ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture + '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $CssScreenshot + '"')
            'set $hover_done = 1'
            'end'
            'if ($css_tick == 485) || ($css_tick == 486)'
            'set variable sControllerPlaybackPads[0].button = 0x4000'
            'end'
        } else {
            'if ($css_tick == 112) || ($css_tick == 113)'
            'set variable sControllerPlaybackPads[0].button = 0x8000'
            'end'
            'if ($css_tick == 150) || ($css_tick == 151)'
            'set variable sControllerPlaybackPads[0].button = 0x0001'
            'end'
            'if ($css_tick == 160) && ($css_mut_printed == 0)'
            'set $css_mut_printed = 1'
            'printf "CPCSSMUT base_diff=%u diff=%u base_stock=%u stock=%u base_costume=%u fkind=%d costume=%u selected=%u back=%u\n", $base_diff, sMNPlayers1PGameLevelValue, $base_stock, sMNPlayers1PGameStockValue, $base_costume, sMNPlayers1PGameSlot.fkind, sMNPlayers1PGameSlot.costume, sMNPlayers1PGameSlot.is_fighter_selected, $backdone'
            'printf "CPCSSVIS enter=%u present=%u base=%u fail=%u mask=%x fkind=%u diff=%u stock=%u time=%u native=%u native_delta=%u\n", gNdsOnePlayerCssNativeEnterCount, gNdsOnePlayerCssNativePresentCount, gNdsOnePlayerCssNativeBaseBlitCount, gNdsOnePlayerCssNativeSurfaceFailCount, gNdsOnePlayerCssNativeVisibleMask, gNdsOnePlayerCssNativeLastFkind, gNdsOnePlayerCssNativeLastDifficulty, gNdsOnePlayerCssNativeLastStock, gNdsOnePlayerCssNativeLastTime, gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.count-$css_native_base'
            ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture + '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $CssScreenshot + '"')
            'end'
            'if ($css_tick == 180) || ($css_tick == 181)'
            'set variable sControllerPlaybackPads[0].button = 0x1000'
            'end'
        }),
        'else',
        'if ($css_tick >= 5) && ($css_tick <= 34)',
        'set variable sControllerPlaybackPads[0].stick_x = 80',
        'end',
        'if ($css_tick == 36) || ($css_tick == 37)',
        'set variable sControllerPlaybackPads[0].button = 0x8000',
        'end',
        'if ($css_tick >= 41) && ($css_tick <= 44)',
        'set variable sControllerPlaybackPads[0].stick_y = -80',
        'end',
        'if ($css_tick == 46) || ($css_tick == 47)',
        'set variable sControllerPlaybackPads[0].button = 0x8000',
        'end',
        'if ($css_tick >= 52) && ($css_tick <= 81)',
        'set variable sControllerPlaybackPads[0].stick_x = -80',
        'set variable sControllerPlaybackPads[0].stick_y = 80',
        'end',
        'if ($css_tick == 82) || ($css_tick == 83)',
        'set variable sControllerPlaybackPads[0].stick_y = 80',
        'end',
        'if ($css_tick == 86) || ($css_tick == 87)',
        'set variable sControllerPlaybackPads[0].button = 0x8000',
        'end',
        'if ($css_tick == 125) || ($css_tick == 126)',
        'set variable sControllerPlaybackPads[0].button = 0x0001',
        'end',
        'if ($css_tick == 135) && ($css_mut_printed == 0)',
        'set $css_mut_printed = 1',
        'printf "CPCSSMUT base_diff=%u diff=%u base_stock=%u stock=%u base_costume=%u fkind=%d costume=%u selected=%u back=%u\n", $base_diff, sMNPlayers1PGameLevelValue, $base_stock, sMNPlayers1PGameStockValue, $base_costume, sMNPlayers1PGameSlot.fkind, sMNPlayers1PGameSlot.costume, sMNPlayers1PGameSlot.is_fighter_selected, $backdone',
        'printf "CPCSSVIS enter=%u present=%u base=%u fail=%u mask=%x fkind=%u diff=%u stock=%u time=%u native=%u native_delta=%u\n", gNdsOnePlayerCssNativeEnterCount, gNdsOnePlayerCssNativePresentCount, gNdsOnePlayerCssNativeBaseBlitCount, gNdsOnePlayerCssNativeSurfaceFailCount, gNdsOnePlayerCssNativeVisibleMask, gNdsOnePlayerCssNativeLastFkind, gNdsOnePlayerCssNativeLastDifficulty, gNdsOnePlayerCssNativeLastStock, gNdsOnePlayerCssNativeLastTime, gNdsRendererNativeFailure.count, gNdsRendererNativeFailure.count-$css_native_base',
        ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture +
         '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $CssScreenshot + '"'),
        'end',
        'if ($css_tick == 160) || ($css_tick == 161)',
        'set variable sControllerPlaybackPads[0].button = 0x1000',
        'end',
        'end',
        'end',
        'end',
        'if (gNdsSceneManagerCurrKind == 8) && ($cssvisits == 1)',
        'set $mode_tick = $mode_tick + 1',
        'if ($mode_tick == 12) || ($mode_tick == 13)',
        'set variable sControllerPlaybackPads[0].button = 0x8000',
        'end',
        'end',
        'end',
        'if (gNdsSceneManagerCurrKind == 17) && (gSYControllerDevices[0].button_tap & 0x8000)',
        'set $saw_css_a = 1',
        'end',
        $(if ($TransitionProof) {
            'if gNdsSceneManagerCurrKind == 51'
            'set $stageclear_frames = $stageclear_frames + 1'
            'if $stageclear_frames == 1'
            'printf "CPTALLY frame=%d stage=%d score=%d bonus_num=%d bonus_id=%d flags=%08x/%08x/%08x allow=%d\n", $stageclear_frames, sSC1PStageClear1PGameStage, sSC1PStageClearScoreTotal, sSC1PStageClearBonusNum, sSC1PStageClearBonusID, sSC1PStageClearBonusFlags[0], sSC1PStageClearBonusFlags[1], sSC1PStageClearBonusFlags[2], sSC1PStageClearIsAllowProceedNext'
            'end'
            'if ($stageclear_frames >= 30) && ($tallyshot == 0)'
            'set $tallyshot = 1'
            'printf "CPTALLY-SHOT frame=%d stage=%d score=%d bonus_num=%d bonus_id=%d flags=%08x/%08x/%08x allow=%d\n", $stageclear_frames, sSC1PStageClear1PGameStage, sSC1PStageClearScoreTotal, sSC1PStageClearBonusNum, sSC1PStageClearBonusID, sSC1PStageClearBonusFlags[0], sSC1PStageClearBonusFlags[1], sSC1PStageClearBonusFlags[2], sSC1PStageClearIsAllowProceedNext'
            ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture +
             '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $TallyScreenshot + '"')
            'end'
            'if (sSC1PStageClearIsAllowProceedNext != 0) && ($tallyfinal == 0)'
            'set $tallyfinal = 1'
            'printf "CPTALLY-FINAL frame=%d stage=%d score=%d bonus_num=%d bonus_id=%d flags=%08x/%08x/%08x allow=%d\n", $stageclear_frames, sSC1PStageClear1PGameStage, sSC1PStageClearScoreTotal, sSC1PStageClearBonusNum, sSC1PStageClearBonusID, sSC1PStageClearBonusFlags[0], sSC1PStageClearBonusFlags[1], sSC1PStageClearBonusFlags[2], sSC1PStageClearIsAllowProceedNext'
            'end'
            'if ($stageclear_frames >= 40) && (($stageclear_frames % 12) <= 1)'
            'set variable sControllerPlaybackPads[0].button = 0x8000'
            'end'
            'end'
            'if gNdsSceneManagerCurrKind == 49'
            'set $continue_frames = $continue_frames + 1'
            'if ($continue_frames >= 60) && (($continue_frames % 30) <= 1)'
            'set variable sControllerPlaybackPads[0].button = 0x8000'
            'end'
            'end'
        }),
        $(if ($IntroScreenshot) {
            'if (gNdsSceneManagerCurrKind == 14) && (dSYTaskmanUpdateCount >= 40) && (gNdsIntroTransientDrawCount >= 20) && ($introshot == 0)'
            'set $introshot = 1'
            'set $intro_enters = gNdsSceneManagerEnterCount'
            'set $intro_rejects = gNdsSceneManagerRejectCount'
            'printf "CPINTROSCENE enters=%u rejects=%u\n", $intro_enters, $intro_rejects'
            'printf "CPINTRO updates=%u submits=%u draws=%u rejects=%u\n", dSYTaskmanUpdateCount, gNdsIntroTransientSubmitCount, gNdsIntroTransientDrawCount, gNdsIntroTransientOwnerRejectCount'
            ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture +
             '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $IntroScreenshot + '"')
            'end'
        }),
        'if (gNdsSceneManagerCurrKind == 14) && (dSYTaskmanUpdateCount >= 180) && ($introaudio == 0)',
        'set $introaudio = 1',
        'printf "CPINTROAUDIO updates=%u sched=%u bgm_calls=%d bgm_id=%d fgm_calls=%d fgm_last=%d bgm_play_delta=%u track=%u bgm_chunks_delta=%u fgm_plays_delta=%u fgm_unsupported_delta=%u\n", dSYTaskmanUpdateCount, sSYSchedulerTicCount, $intro_bgm_calls, $intro_bgm_id, $intro_fgm_calls, $intro_fgm_last, gNdsAudioBgmPlayCalls-$intro_bgm_play_base, gNdsAudioBgmTrackID, gNdsAudioBgmChunkPlayCount-$intro_bgm_base, gNdsAudioFgmSupportedPlayCount-$intro_fgm_base, gNdsAudioFgmUnsupportedCallCount-$intro_fgm_unsupported_base',
        'end',
        # Asset-loading/countdown presents are tracked, but only GO-state
        # presents satisfy BattlePresents.
        'if $inbattle && (gNdsSceneManagerCurrKind == 52) && (dSYTaskmanUpdateCount != 0)',
        'if $battleframes == 0',
        'set $startup_free = (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr',
        'set $startup_guest_min = gNdsTaskmanGeneralHeapFreeMin',
        'set $battle_arena = gNdsSceneManagerArenaSize',
        'printf "CPSTARTUP free=%u guest_min=%u arena=%u\n", $startup_free, $startup_guest_min, $battle_arena',
        'end',
        'set $battleframes = $battleframes + 1',
        'set $free_now = (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr',
        'if $free_now < $active_min',
        'set $active_min = $free_now',
        'end',
        'if gNdsK0BattleInGo != 0',
        'if $saw_go == 0',
        'set $saw_go = 1',
        'printf "CPGO total_present=%d pacing_present=%u updates=%u\n", $battleframes, gNdsBattlePlayablePacingPresentedFrames, dSYTaskmanUpdateCount',
        ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture +
         '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' + $GoScreenshot + '"'),
        'end',
        'set $goframes = $goframes + 1',
        # Real post-GO human input while the source CPU remains enabled.
        'if $transition == 0',
        'if ($goframes >= 60) && ($goframes <= 90)',
        'set variable sControllerPlaybackPads[0].stick_x = 80',
        'end',
        'if ($goframes == 120) || ($goframes == 121)',
        'set variable sControllerPlaybackPads[0].button = 0x8000',
        'end',
        'if ($goframes == 180) || ($goframes == 181)',
        'set variable sControllerPlaybackPads[0].button = 0x0008',
        'end',
        'if ($goframes == 240) || ($goframes == 241)',
        'set variable sControllerPlaybackPads[0].button = 0x4000',
        'end',
        'else',
        # Keep the source CPU enabled and exhaust its stock through ordinary
        # controller input only. Oscillation keeps Mario near the engagement
        # zone while repeated direction+A attacks continue until battle ends.
        'if (($goframes % 90) < 45)',
        'set variable sControllerPlaybackPads[0].stick_x = 80',
        'else',
        'set variable sControllerPlaybackPads[0].stick_x = -80',
        'end',
        'if (($goframes % 8) <= 1)',
        'set variable sControllerPlaybackPads[0].button = 0x8000',
        'end',
        'if (($goframes % 120) >= 54) && (($goframes % 120) <= 61)',
        'set variable sControllerPlaybackPads[0].stick_y = 80',
        'set variable sControllerPlaybackPads[0].button = 0x8000',
        'end',
        'end',
        'end',
        ('if ($transition == 0) && ($goframes >= ' + $BattlePresents + ')')
    ) + $stopLines + @(
        'printf "CPPACKAUDIT failure=%u failure_kind=%u extern_fighters=%u extern_patches=%u\n", gNdsPreviewPackFailure, gNdsPreviewPackFailureKind, $extern_fighters, gNdsBattleCoreExternPatchCount',
        'printf "CPSCENE mgrkind=%u intro_enters=%u final_enters=%u enter_delta=%u intro_rejects=%u final_rejects=%u reject_delta=%u\n", gNdsSceneManagerCurrKind, $intro_enters, gNdsSceneManagerEnterCount, gNdsSceneManagerEnterCount-$intro_enters, $intro_rejects, gNdsSceneManagerRejectCount, gNdsSceneManagerRejectCount-$intro_rejects',
        'printf "CPFRAME renderframe=%u updates=%u\n", gNdsRendererProfileFrameCount, dSYTaskmanUpdateCount',
        'printf "CPRAM free=%u used=%u images=%u imagebytes=%u\n", (unsigned)gSYTaskmanGeneralHeap.end-(unsigned)gSYTaskmanGeneralHeap.ptr, (unsigned)gSYTaskmanGeneralHeap.ptr-(unsigned)gSYTaskmanGeneralHeap.start, gNdsNativeOwnerImageLoadCount, gNdsNativeOwnerImageBytes',
        'printf "CPHEAP startup_free=%u startup_guest_min=%u active_free_min=%u guest_free_min=%u arena=%u\n", $startup_free, $startup_guest_min, $active_min, gNdsTaskmanGeneralHeapFreeMin, $battle_arena',
        'printf "CPPACK loaded=%u bytes=%u extern_patches=%u extern_loads=%u foreign_bytes=%u foreign_loads=%u\n", gNdsPreviewPackLoadCount, gNdsPreviewPackDataBytes, gNdsBattleCoreExternPatchCount, gNdsBattleCoreExternLoadCount, gNdsBattleCoreForeignImageBytes, gNdsBattleCoreForeignImageLoadCount',
        'printf "CPSTATE active=%08x onep=%08x transfer=%08x vs=%08x owner1p=%u applied=%u refused=%u setup=%d\n", gSCManagerBattleState, &gSCManager1PGameBattleState, &gSCManagerTransferBattleState, &gSCManagerVSBattleState, gSCManagerBattleState == &gSCManager1PGameBattleState, gNdsSC1PGameBridgeAppliedCount, gNdsSC1PGameBridgeRefusedCount, $setupcount',
        'printf "CPFRAMES total=%d go=%d pacing_present=%u logic=%u\n", $battleframes, $goframes, gNdsBattlePlayablePacingPresentedFrames, gNdsBattlePlayablePacingLogicFrames',
        'printf "CPINPUT saw_css_a=%u back=%u cssvisits=%d introaudio=%u intro_bgm_calls=%d intro_bgm_id=%d intro_fgm_calls=%d intro_fgm_last=%d\n", $saw_css_a, $backdone, $cssvisits, $introaudio, $intro_bgm_calls, $intro_bgm_id, $intro_fgm_calls, $intro_fgm_last',
        ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture +
         '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' +
         $Screenshot + '"'),
        'printf "CPDONE n=%d\n", $n',
        'detach',
        'quit',
        'end',
        'end',
        'continue',
        'end',
        $(if ($symbols -contains 'ndsSyMallocOverflowHalt') {
            'break ndsSyMallocOverflowHalt'
        }),
        $(if (@($symbols -match '^ndsPreviewPackLoadHalt(?:\.|$)').Count -ne 0) {
            'break ndsPreviewPackLoadHalt'
        }),
        $(if ($symbols -contains '__excpt_entry') { 'break __excpt_entry' }),
        # Starts the run; returns at the battle screenshot or the Hits-th
        # stop. A guest the walk cannot steer never reaches either and the
        # run ends by timeout -- verdict BLOCKED, not FAILED.
        'continue',
        'printf "CPSTOP n=%d pc=%08x cpsr=%08x\n", $n, $pc, $cpsr',
        'info symbol $pc',
        'printf "CPREGISTERS r0=%08x r1=%08x lr=%08x cpsr=%08x\n", $r0, $r1, $lr, $cpsr',
        'printf "CPOOM request=%u free=%u align=%u caller=%08x\n", gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom, gNdsSyMallocOverflowAlignment, gNdsSyMallocOverflowCallerLR',
        $(if ($symbols -contains 'gNdsPreviewPackLoadCount') {
            'printf "CPPACK loaded=%u bytes=%u failure=%u kind=%u\n", gNdsPreviewPackLoadCount, gNdsPreviewPackDataBytes, gNdsPreviewPackFailure, gNdsPreviewPackFailureKind'
        }),
        'bt 10',
        'detach',
        'quit'
    )

    try {
        Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
            -ScriptName 'p2_campaign_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds | Out-Null
    }
    catch {
        # Timeout here means the guest parked: no new scene entries, no
        # battle, nothing crashed (a crash trips the abort verdict below on
        # the partial capture). Keep the capture and report BLOCKED.
        if ("$_" -match 'timed out after') { $timedOut = $true }
        else { throw }
    }
}
finally {
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}

# --- Verdict (reads the capture file, never the helper's return) -----------
if (-not (Test-Path -LiteralPath $Artifact -PathType Leaf)) {
    Write-Output 'VERDICT: FAILED no-capture (no GDB transcript; see runner logs)'
    exit 1
}
$text = Get-Content -LiteralPath $Artifact -Raw
$failReasons = @()

# BreakOnStartup normally stops at 0xfffffffc before symbolized ARM9 startup.
# Other unsymbolized stops or an abort/undefined CPU mode are failures.
$terminal = [regex]::Match($text, 'CPSTOP n=\d+ pc=[0-9a-fA-F]+ cpsr=([0-9a-fA-F]+)')
$terminalMode = if ($terminal.Success) {
    [Convert]::ToUInt32($terminal.Groups[1].Value, 16) -band 0x1f
} else { 0 }
if (($text -match '(?m)^0x(?!fffffffc)[0-9a-fA-F]+ in \?\? \(\)') -or
    ($text -match 'Program received signal SIG(?:ILL|SEGV|BUS)') -or
    ($terminalMode -in @(0x17, 0x1b))) {
    $failReasons += 'cpu-abort-signature'
}
if ($text -match '(?m)^ndsSyMallocOverflowHalt in section') {
    $failReasons += 'general-heap-overflow'
}
if ($text -match '(?m)^ndsPreviewPackLoadHalt(?:\S*) in section') {
    $failReasons += 'preview-pack-refusal'
}
foreach ($match in [regex]::Matches($text, '(?m)^CPGFX \d+ peak=\d+ capacity=\d+ overflow=(\d+) noroom=(\d+) dl_overflow=(\d+)')) {
    if ([uint32]$match.Groups[1].Value -ne 0 -or
        [uint32]$match.Groups[2].Value -ne 0 -or
        [uint32]$match.Groups[3].Value -ne 0) {
        $failReasons += 'graphics-buffer-refusal'
    }
}
# ArenaAllocFailCount counts unsuccessful sizing probes before a successful
# arena allocation (diagnostics_taskman_heap.c), not fatal task allocations.
# Preserve the value in CPERR; the overflow trap detects real exhaustion.
foreach ($line in @([regex]::Matches($text, '(?m)^CPERR \d+ allocfail=(\d+) openfail=(\d+) formatfail=(\d+) fixupfail=(\d+).*$'))) {
    if (([uint32]$line.Groups[2].Value -ne 0) -or
        ([uint32]$line.Groups[3].Value -ne 0) -or ([uint32]$line.Groups[4].Value -ne 0)) {
        # Fatal OOM/asset failure: fail fast, do not route-judge a sick guest.
        $failReasons += ('error-counters: ' + $line.Value.Trim())
        break
    }
}
# Startup intentionally uses its source overlay arena outside this registry;
# allow its one entry only when the capture actually saw Startup (kind 27).
$startupEntries = [regex]::Matches($text, '(?m)^CPLINE \d+ curr=27 ').Count
$mismatches = @([regex]::Matches($text, '(?m)^CPLINE \d+ curr=\d+ prev=\d+ enters=\d+ rej=(\d+) unreg=(\d+) mism=(\d+).*$') |
    Where-Object { ([uint32]$_.Groups[1].Value -ne 0) -or ([uint32]$_.Groups[2].Value -gt $startupEntries) -or ([uint32]$_.Groups[3].Value -ne 0) } |
    Select-Object -First 1)
if ($mismatches.Count -gt 0) {
    $failReasons += ('registry: ' + $mismatches.Value.Trim())
}
if ($failReasons.Count -gt 0) {
    Write-Output ('VERDICT: FAILED ' + ($failReasons -join ' | '))
    exit 1
}

$scenes = @([regex]::Matches($text, '(?m)^CPLINE \d+ curr=(\d+) prev=(\d+).*$') |
    ForEach-Object { [int]$_.Groups[1].Value })
Write-Output ('route scenes: ' + ($scenes -join ' -> '))

if ($FullRosterHover) {
    $draw = [regex]::Match($text, '(?m)^CPHOVER_DRAW lap1=fff lap2=fff rejects=0 arena=\d+\s*$')
    $retired = [regex]::Match($text, '(?m)^CPHOVER_EXIT resident_kind=28 main=0 model=0\s*$')
    $hover = [regex]::Match($text,
        '(?m)^CPHOVER mask=([0-9a-fA-F]+) loads=(\d+) bytes=(\d+) failure=(\d+) kind=(\d+) overflow=(\d+) free=(\d+) min=(\d+) native_delta=(\d+)\s*$',
        [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
    if ($hover.Success) {
        Write-Output ('1P hover: mask=0x{0} loads={1} bytes={2} free={3} min={4}' -f
            $hover.Groups[1].Value, $hover.Groups[2].Value,
            $hover.Groups[3].Value, $hover.Groups[7].Value,
            $hover.Groups[8].Value)
    }
    if ($hover.Success -and $draw.Success -and $retired.Success -and
        ([Convert]::ToUInt32($hover.Groups[1].Value, 16) -eq 0xfffu) -and
        ([uint32]$hover.Groups[4].Value -eq 0u) -and
        ([uint32]$hover.Groups[6].Value -eq 0u) -and
        ([uint32]$hover.Groups[7].Value -gt 0u) -and
        ([uint32]$hover.Groups[9].Value -eq 0u)) {
        Write-Output 'VERDICT: PASS 1P CSS full-roster hover walk.'
        exit 0
    }
    Write-Output 'VERDICT: BLOCKED 1P CSS full-roster hover walk incomplete.'
    if ($timedOut) { Write-Output 'note: hover run ended at its timeout ceiling.' }
    exit 2
}

if ($TransitionProof) {
    $winRoute = [regex]::Match($text, '(?m)^CPWINROUTE .+$',
        [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
    $stageClear = [regex]::Match($text, '(?m)^CPSTAGECLEAR-ENTER .+$',
        [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
    $tally = [regex]::Match($text, '(?m)^CPTALLY-FINAL .+$',
        [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
    $tallyShot = [regex]::Match($text, '(?m)^CPTALLY-SHOT .+$',
        [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
    $nextIntro = [regex]::Match($text, '(?m)^CPNEXTINTRO .+$',
        [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
    $nextBattle = [regex]::Match($text, '(?m)^CPNEXTBATTLE .+$',
        [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
    $transitionDone = [regex]::Match($text,
        '(?m)^CPTRANSITIONDONE n=\d+ stageclear=1 continue=(\d+)\s*$',
        [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
    $tallyShotOk = (-not [string]::IsNullOrWhiteSpace($TallyScreenshot) -and
        (Test-Path -LiteralPath $TallyScreenshot -PathType Leaf))
    $transitionRouteOk = (($scenes -join ',') -match '52,51,14,52')
    $nextHealth = if ($nextBattle.Success) {
        [regex]::Match($nextBattle.Value,
            'heap_min=(\d+) native_fail=(\d+) native_transition_delta=(\d+) drive_frames=(\d+) attacks=(\d+)')
    } else { [regex]::Match('', 'x') }
    $transitionHealthOk = ($nextHealth.Success -and
        ([uint32]$nextHealth.Groups[1].Value -ge 25600u) -and
        ([uint32]$nextHealth.Groups[3].Value -eq 0u) -and
        ([uint32]$nextHealth.Groups[4].Value -gt 0u) -and
        ([uint32]$nextHealth.Groups[5].Value -gt 0u))
    if ($winRoute.Success -and $stageClear.Success -and $tally.Success -and
        $tallyShot.Success -and $transitionRouteOk -and $transitionHealthOk -and
        $nextIntro.Success -and $nextBattle.Success -and $transitionDone.Success -and
        $tallyShotOk) {
        Write-Output ('win route: ' + $winRoute.Value.Trim())
        Write-Output ('tally: ' + $tally.Value.Trim())
        Write-Output ('next: ' + $nextBattle.Value.Trim())
        Write-Output ('VERDICT: PASS natural 1P win -> StageClear -> next 1P Game; tally=' +
            $TallyScreenshot)
        exit 0
    }
    Write-Output 'VERDICT: BLOCKED 1P transition proof incomplete.'
    if (-not $winRoute.Success) { Write-Output 'seam: source manager never called sc1PStageClearStartScene.' }
    elseif (-not $stageClear.Success) { Write-Output 'seam: StageClear call did not enter registered scene 51.' }
    elseif (-not $tally.Success) { Write-Output 'seam: source StageClear tally never reached allow-proceed state.' }
    elseif (-not $tallyShot.Success) { Write-Output 'seam: guest tally proof stop was never reached.' }
    elseif (-not $transitionRouteOk) { Write-Output 'seam: scene entries did not contain 52 -> 51 -> 14 -> 52.' }
    elseif (-not $transitionHealthOk) { Write-Output 'seam: transition heap/native/guest-input health gate failed.' }
    elseif (-not $nextIntro.Success) { Write-Output 'seam: source manager did not route StageClear to the next intro.' }
    elseif (-not $nextBattle.Success) { Write-Output 'seam: next 1P Game was not entered after StageClear.' }
    elseif (-not $tallyShotOk) { Write-Output ('seam: tally capture missing: ' + $TallyScreenshot) }
    if ($text -match '(?m)^CPLOSSROUTE') {
        Write-Output 'note: the same run also reached the source Continue call after a natural loss.'
    }
    if ($timedOut) { Write-Output 'note: transition run ended at its timeout ceiling.' }
    exit 2
}

$saw1PMode = ($scenes -contains 8)
$saw1PCss = ($scenes -contains 17)
$sawBattle = ($text -match '(?m)^CPBATTLE-HIT')
$shot = ($text -match '(?m)^CPFRAME')
$routeText = $scenes -join ','
$routeOk = ($routeText -match '1,7,8,17,8,17,14,52')

$modeVis = [regex]::Match($text,
    '(?m)^CPMODEVIS source=(\d+) enter=(\d+) present=(\d+) base=(\d+) fail=(\d+) mask=([0-9a-fA-F]+) option=(\d+) selected=(\d+) native=(\d+) native_delta=(\d+)\s*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$modeShotOk = (Test-Path -LiteralPath $ModeScreenshot -PathType Leaf)
$modeNativeOk = ($modeVis.Success -and
    ([uint32]$modeVis.Groups[2].Value -ge 1u) -and
    ([uint32]$modeVis.Groups[3].Value -ge 4u) -and
    ([uint32]$modeVis.Groups[4].Value -ge 1u) -and
    ([uint32]$modeVis.Groups[5].Value -eq 0u) -and
    ([Convert]::ToUInt32($modeVis.Groups[6].Value, 16) -ne 0u) -and
    ($modeVis.Groups[1].Value -eq $modeVis.Groups[7].Value) -and
    ($modeVis.Groups[7].Value -eq '0') -and
    ($modeVis.Groups[8].Value -eq '0') -and
    ([uint32]$modeVis.Groups[10].Value -eq 0u) -and $modeShotOk)
if ($modeVis.Success) {
    Write-Output ('1P Mode native: source={0} entries={1} presents={2} base={3} fail={4} mask=0x{5} option={6} selected={7} native={8} native_delta={9}' -f
        $modeVis.Groups[1].Value, $modeVis.Groups[2].Value,
        $modeVis.Groups[3].Value, $modeVis.Groups[4].Value,
        $modeVis.Groups[5].Value, $modeVis.Groups[6].Value,
        $modeVis.Groups[7].Value, $modeVis.Groups[8].Value,
        $modeVis.Groups[9].Value, $modeVis.Groups[10].Value)
}

$input = [regex]::Match($text,
    '(?m)^CPINPUT saw_css_a=(\d+) back=(\d+) cssvisits=(\d+) introaudio=(\d+) intro_bgm_calls=(\d+) intro_bgm_id=(-?\d+) intro_fgm_calls=(\d+) intro_fgm_last=(-?\d+)\s*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$inputOk = ($input.Success -and ([uint32]$input.Groups[1].Value -eq 1) -and
    ([uint32]$input.Groups[2].Value -eq 1) -and ([uint32]$input.Groups[3].Value -ge 2) -and
    ([uint32]$input.Groups[4].Value -eq 1))

$mut = [regex]::Match($text,
    '(?m)^CPCSSMUT base_diff=(\d+) diff=(\d+) base_stock=(\d+) stock=(\d+) base_costume=(\d+) fkind=(-?\d+) costume=(\d+) selected=(\d+) back=(\d+)\s*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$menuChanged = ($mut.Success -and ($mut.Groups[1].Value -ne $mut.Groups[2].Value) -and
    ($mut.Groups[3].Value -ne $mut.Groups[4].Value) -and
    ($mut.Groups[5].Value -ne $mut.Groups[7].Value) -and
    ($mut.Groups[6].Value -eq '0') -and ($mut.Groups[8].Value -eq '1') -and
    ($mut.Groups[9].Value -eq '1'))
if ($mut.Success) {
    Write-Output ('CSS changes: difficulty {0}->{1}, stock {2}->{3}, costume {4}->{5}, fkind={6}, back={7}' -f
        $mut.Groups[1].Value, $mut.Groups[2].Value, $mut.Groups[3].Value,
        $mut.Groups[4].Value, $mut.Groups[5].Value, $mut.Groups[7].Value,
        $mut.Groups[6].Value, $mut.Groups[9].Value)
}

$cssVis = [regex]::Match($text,
    '(?m)^CPCSSVIS enter=(\d+) present=(\d+) base=(\d+) fail=(\d+) mask=([0-9a-fA-F]+) fkind=(\d+) diff=(\d+) stock=(\d+) time=(\d+) native=(\d+) native_delta=(\d+)\s*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$cssShotOk = (Test-Path -LiteralPath $CssScreenshot -PathType Leaf)
$cssNativeOk = ($cssVis.Success -and
    ([uint32]$cssVis.Groups[1].Value -ge 2u) -and
    ([uint32]$cssVis.Groups[2].Value -gt 0u) -and
    ([uint32]$cssVis.Groups[3].Value -ge 2u) -and
    ([uint32]$cssVis.Groups[4].Value -eq 0u) -and
    ([Convert]::ToUInt32($cssVis.Groups[5].Value, 16) -ne 0u) -and
    ($cssVis.Groups[6].Value -eq '0') -and
    ($cssVis.Groups[7].Value -eq $mut.Groups[2].Value) -and
    ($cssVis.Groups[8].Value -eq $mut.Groups[4].Value) -and
    ([uint32]$cssVis.Groups[11].Value -eq 0u) -and $cssShotOk)
if ($cssVis.Success) {
    Write-Output ('1P CSS native: entries={0} presents={1} base={2} fail={3} mask=0x{4} fkind={5} diff={6} stock={7} time={8} native={9} native_delta={10}' -f
        $cssVis.Groups[1].Value, $cssVis.Groups[2].Value,
        $cssVis.Groups[3].Value, $cssVis.Groups[4].Value,
        $cssVis.Groups[5].Value, $cssVis.Groups[6].Value,
        $cssVis.Groups[7].Value, $cssVis.Groups[8].Value,
        $cssVis.Groups[9].Value, $cssVis.Groups[10].Value,
        $cssVis.Groups[11].Value)
}

# The 1P-owned select state names the fighter and menu state actually committed.
$css1p = [regex]::Match($text,
    '(?m)^CP1P \d+ player=(\d+) fkind=(\d+) costume=(\d+) diff=(\d+) stocks=(\d+) stage=(\d+).*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$marioCommitted = ($css1p.Success -and ($css1p.Groups[2].Value -eq '0') -and
    ($css1p.Groups[3].Value -eq $mut.Groups[7].Value) -and
    ($css1p.Groups[4].Value -eq $mut.Groups[2].Value) -and
    ($css1p.Groups[5].Value -eq $mut.Groups[4].Value) -and
    ($css1p.Groups[6].Value -eq '0'))
if ($css1p.Success) {
    Write-Output ('1P select: player={0} fkind={1} costume={2} difficulty={3} stocks={4} stage={5}' -f
        $css1p.Groups[1].Value, $css1p.Groups[2].Value, $css1p.Groups[3].Value,
        $css1p.Groups[4].Value, $css1p.Groups[5].Value, $css1p.Groups[6].Value)
}

$hit = [regex]::Match($text,
    '(?m)^CPBATTLE \d+ gametype=(\d+) gkind=([0-9a-fA-F]+) time=(\d+) pl=(\d+) cp=(\d+) s0=(\d+)/(\d+)/(\d+) s1=(\d+)/(\d+)/(\d+) s2=(\d+)/(\d+)/(\d+) s3=(\d+)/(\d+)/(\d+).*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$battleContentOk = $false
if ($hit.Success) {
    $kinds = @($hit.Groups[6].Value, $hit.Groups[9].Value,
               $hit.Groups[12].Value, $hit.Groups[15].Value)
    $battleContentOk = (($hit.Groups[2].Value -eq '04') -and
        ($hit.Groups[4].Value -eq '1') -and ($hit.Groups[5].Value -eq '1') -and
        ($kinds -contains '0') -and ($kinds -contains '5'))
    Write-Output ('battle: gametype={0} gkind=0x{1} pl={2} cp={3} kinds={4}' -f
        $hit.Groups[1].Value, $hit.Groups[2].Value, $hit.Groups[4].Value,
        $hit.Groups[5].Value, ($kinds -join ','))
}

$state = [regex]::Match($text,
    '(?m)^CPSTATE active=[0-9a-fA-F]+ onep=[0-9a-fA-F]+ transfer=[0-9a-fA-F]+ vs=[0-9a-fA-F]+ owner1p=(\d+) applied=(\d+) refused=(\d+) setup=(\d+)\s*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$stateOk = ($state.Success -and ($state.Groups[1].Value -eq '1') -and
    ($state.Groups[2].Value -eq '1') -and ($state.Groups[3].Value -eq '0') -and
    ($state.Groups[4].Value -eq '1'))

$frames = [regex]::Match($text,
    '(?m)^CPFRAMES total=(\d+) go=(\d+) pacing_present=(\d+) logic=(\d+)\s*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$framesOk = ($frames.Success -and
    ([uint32]$frames.Groups[2].Value -ge [uint32]$BattlePresents))

$heap = [regex]::Match($text,
    '(?m)^CPHEAP startup_free=(\d+) startup_guest_min=(\d+) active_free_min=(\d+) guest_free_min=(\d+) arena=(\d+)\s*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$heapOk = ($heap.Success -and ([uint64]$heap.Groups[3].Value -lt [uint64]0xffffffff) -and
    ([uint64]$heap.Groups[5].Value -gt 0))
if ($heap.Success) {
    Write-Output ('heap: startup-free={0} startup-global-min={1} active-min={2} guest-min={3} arena={4}' -f
        $heap.Groups[1].Value, $heap.Groups[2].Value, $heap.Groups[3].Value,
        $heap.Groups[4].Value, $heap.Groups[5].Value)
}

$introBgmCall = [regex]::Match($text,
    '(?m)^CPINTRO-BGM call=(\d+) id=(\d+)\s*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$introFgmCall = [regex]::Match($text,
    '(?m)^CPINTRO-FGM call=(\d+) id=(\d+)\s*$',
    [System.Text.RegularExpressions.RegexOptions]::RightToLeft)
$introAudioOk = ($introBgmCall.Success -and $introFgmCall.Success -and
    ([uint32]$introBgmCall.Groups[1].Value -ge 1) -and
    ([uint32]$introBgmCall.Groups[2].Value -eq 35) -and
    ([uint32]$introFgmCall.Groups[1].Value -ge 3))
$introShotOk = ([string]::IsNullOrWhiteSpace($IntroScreenshot) -or
    (Test-Path -LiteralPath $IntroScreenshot -PathType Leaf))
$goShotOk = (Test-Path -LiteralPath $GoScreenshot -PathType Leaf)
$battleShotOk = (Test-Path -LiteralPath $Screenshot -PathType Leaf)

if ($sawBattle -and $shot -and $saw1PMode -and $saw1PCss -and $routeOk -and
    $modeNativeOk -and $inputOk -and $menuChanged -and $cssNativeOk -and
    $marioCommitted -and $battleContentOk -and
    $stateOk -and $framesOk -and $heapOk -and $introAudioOk -and
    $introShotOk -and $goShotOk -and $battleShotOk) {
    Write-Output ('VERDICT: PASS 1P-Mario-vs-Link-Hyrule source-route GO-frames=' +
        $frames.Groups[2].Value + ' frame=' + $Screenshot)
    exit 0
}

# No invented success: name the owning seam and stop.
Write-Output 'VERDICT: BLOCKED 1P prefix did not satisfy the full Mario-vs-Link contract.'
if (-not $saw1PMode) {
    Write-Output ('seam: ModeSelect exit is owned by the shell walk ' +
        '(src/nds/nds_menu_shell_core.c kNdsMenuWalkMode1P steers A on the ' +
        'opening 1P GAME cursor under gNdsMenuShellWalkRoute=1). No 1PMode ' +
        'entry means that tap never committed: check the input-ring lines ' +
        'and ndsSceneManagerFind(nSCKind1PMode) registration in this build.')
} elseif (-not $saw1PCss) {
    Write-Output ('seam: 1PMode reached but no 1P CSS entry. The A tap at ' +
        'driver tic 12 is owned by ndsMenuShellWalkDrive1PSourceMenus ' +
        '(src/nds/nds_menu_shell_core.c) through the playback pads; ' +
        'CPCTL published=0 means the edge never published, otherwise the ' +
        'scene_prev/InitVars option state refused it.')
} elseif (-not $modeNativeOk) {
    Write-Output ('seam: native 1P Mode presentation/capture failed; inspect CPMODEVIS and ' + $ModeScreenshot)
} elseif (-not $menuChanged) {
    Write-Output 'seam: source 1P CSS input did not preserve difficulty/stock/costume changes; inspect CPCSSBASE/CPCSSMUT.'
} elseif (-not $cssNativeOk) {
    Write-Output ('seam: native 1P CSS presentation/capture failed; inspect CPCSSVIS and ' + $CssScreenshot)
} elseif (-not $marioCommitted) {
    Write-Output 'seam: source 1P CSS did not commit Mario plus the changed menu state; inspect CP1P/CPCSSMUT.'
} elseif (-not $introAudioOk) {
    Write-Output 'seam: 1P intro entered but BGM/voice evidence is incomplete; inspect CPINTROAUDIO and audio owner counters.'
} elseif (-not $battleContentOk) {
    Write-Output 'seam: source campaign battle did not resolve Mario + Link on Hyrule; inspect CPBATTLE and sc1PGameSetupStageAll.'
} elseif (-not $stateOk) {
    Write-Output 'seam: campaign battle state ownership/setup count failed; CPSTATE must show 1P owner, applied=1, refused=0, setup=1.'
} elseif (-not $framesOk) {
    Write-Output ('seam: GO was reached but fewer than ' + $BattlePresents + ' active fight presents completed; inspect CPGO/CPFRAMES.')
} else {
    Write-Output 'seam: route/state passed but capture/resource evidence is incomplete; inspect CPHEAP/CPINTRO/CPFRAME and PNG paths.'
}
if ($timedOut) {
    Write-Output 'note: no later scene entry was captured before timeout; this does not establish whether the guest was updating or hung.'
}
exit 2
