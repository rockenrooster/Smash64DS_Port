param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [string]$Harness = 'battle_mariofox_preview_loop',
    [string]$Target = 'smash64ds-battle-mariofox-preview-loop',
    [string]$Build = 'build-battle-mariofox-preview-loop-harness',
    [int]$ExpectedMode = 51,
    [int]$ExpectedHarnessSceneCurr = 22,
    [int]$ExpectedHarnessScenePrev = 21,
    [string]$Label = 'Battle Mario/Fox preview-loop',
    [string]$HarnessSelectMessage = 'Direct preview-loop harness did not select VSBattle from Maps.'
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Join-Path $root "$Target.nds"
$elf = Join-Path $root "$Target.elf"
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir "melonds.$($Harness.Replace('_','-'))-harness.stdout.log"
$stderr = Join-Path $logDir "melonds.$($Harness.Replace('_','-'))-harness.stderr.log"
$scriptName = "_$($Harness)_harness.gdb"
$configState = $null
$emulator = $null
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
function Get-Ints {
    param([System.Text.RegularExpressions.Match]$Match)
    $values = @()
    for ($i = 1; $i -lt $Match.Groups.Count; $i++) {
        $text = $Match.Groups[$i].Value
        if ($text -like '0x*') { $values += [int64](Convert-MarkerUInt32 $text) }
        else { $values += [int64]$text }
    }
    return $values
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=$Target BUILD=$Build NDS_DEV_SCENE_HARNESS=$Harness -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw "$Label harness build did not produce the expected ROM and ELF."
}
if (-not (Test-Path $melonDsPath)) { throw "melonDS executable not found: $melonDsPath" }
if (-not (Test-Path $Gdb)) { throw "GDB executable not found: $Gdb" }
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $melonDsPath -GdbPort $verifierContext.GdbPort -Persistent:([bool]$verifierContext.PersistentConfig)
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $verifierContext.GdbPort | Out-Null
    Start-Sleep -Seconds ([Math]::Max($DelaySeconds, 1))
    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 5',
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind',
        'printf "VS_TRANS=%#x,%#x,%u,%u,%u\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask, gNdsVSModeStartTransitionSceneCurrFinal, gNdsVSModeStartTransitionScenePrevFinal, gNdsVSModeStartTransitionCleanupCount',
        'printf "PV_TRANS=%#x,%#x,%u,%#x,%u,%u,%u,%u,%u,%u\n", gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask, gNdsPlayersVSReadyTransitionUpdateCount, gNdsPlayersVSReadyTransitionInputMask, gNdsPlayersVSReadyTransitionScenePrevBefore, gNdsPlayersVSReadyTransitionSceneCurrBefore, gNdsPlayersVSReadyTransitionScenePrevFinal, gNdsPlayersVSReadyTransitionSceneCurrFinal, gNdsPlayersVSReadyTransitionPlayerCount, gNdsPlayersVSReadyTransitionStageSelect',
        'printf "MAPS_TRANS=%#x,%#x,%u,%#x,%u,%u,%u,%u,%u\n", gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask, gNdsMapsSelectTransitionUpdateCount, gNdsMapsSelectTransitionInputMask, gNdsMapsSelectTransitionScenePrevBefore, gNdsMapsSelectTransitionSceneCurrBefore, gNdsMapsSelectTransitionScenePrevFinal, gNdsMapsSelectTransitionSceneCurrFinal, gNdsMapsSelectTransitionSelectedGKind',
        'printf "SCHED_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxSchedulerLoopResult, gNdsFighterMarioFoxSchedulerLoopSafeResult, gNdsFighterMarioFoxSchedulerLoopMask, gNdsFighterMarioFoxSchedulerLoopDeferredMask, gNdsFighterMarioFoxSchedulerLoopCount',
        'printf "CTRL_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxControllerLoopResult, gNdsFighterMarioFoxControllerLoopSafeResult, gNdsFighterMarioFoxControllerLoopMask, gNdsFighterMarioFoxControllerLoopDeferredMask, gNdsFighterMarioFoxControllerLoopCount',
        'printf "PREV_LOOP=%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxPreviewLoopResult, gNdsFighterMarioFoxPreviewLoopSafeResult, gNdsFighterMarioFoxPreviewLoopMask, gNdsFighterMarioFoxPreviewLoopDeferredMask, gNdsFighterMarioFoxPreviewLoopCount, gNdsFighterPreviewLoopFrameMax, gNdsFighterPreviewLoopUpdateMax',
        'printf "PREV_BACKEND=%u,%#x,%u,%u,%u,%u,%u,%d,%d,%d,%d\n", gNdsControllerPlaybackEnabled, gNdsControllerPlaybackConnectedMask, gNdsControllerPlaybackFrameCount, gNdsControllerPlaybackReadCount, gNdsControllerLiveReadCount, gNdsFighterPreviewLoopSYReadCount, gNdsFighterPreviewLoopSYUpdateCount, gNdsControllerPlaybackPad0StickX, gNdsControllerPlaybackPad1StickX, gNdsControllerPlaybackPad0StickY, gNdsControllerPlaybackPad1StickY',
        'printf "PREV_TASKMAN=%u,%u,%u,%u,%u,%u\n", gNdsFighterPreviewLoopPrepared, gNdsFighterPreviewLoopTaskmanUpdateCount, gNdsFighterPreviewLoopVSBattleUpdateCount, gNdsFighterPreviewLoopBaseVSBattleUpdateCount, gNdsFighterPreviewLoopSchedulerUpdateCount, gNdsTaskmanBoundedUpdateCount',
        'printf "PREV_PROCESS=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterPreviewLoopP0ProcessAttachCount, gNdsFighterPreviewLoopP1ProcessAttachCount, gNdsFighterPreviewLoopP0GObjProcessRunCount, gNdsFighterPreviewLoopP1GObjProcessRunCount, gNdsFighterPreviewLoopP0ProcCallbackCount, gNdsFighterPreviewLoopP1ProcCallbackCount, gNdsFighterPreviewLoopProcessAttachEscapeCount',
        'printf "PREV_INPUT=%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterPreviewLoopP0PlaybackApplyCount, gNdsFighterPreviewLoopP1PlaybackApplyCount, gNdsFighterPreviewLoopP0ControllerToFTInputCount, gNdsFighterPreviewLoopP1ControllerToFTInputCount, gNdsFighterPreviewLoopP0DirectFTInputWriteCount, gNdsFighterPreviewLoopP1DirectFTInputWriteCount, gNdsFighterPreviewLoopP0DashTapEligibleCount, gNdsFighterPreviewLoopP1DashTapEligibleCount, gNdsFighterPreviewLoopP0ButtonTapMask, gNdsFighterPreviewLoopP1ButtonTapMask, gNdsFighterPreviewLoopP0ButtonHoldMask, gNdsFighterPreviewLoopP1ButtonHoldMask, gNdsFighterPreviewLoopP0JumpButtonTapCount, gNdsFighterPreviewLoopP1JumpButtonTapCount',
        'printf "PREV_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterPreviewLoopP0StatusStart, gNdsFighterPreviewLoopP1StatusStart, gNdsFighterPreviewLoopP0MotionStart, gNdsFighterPreviewLoopP1MotionStart, gNdsFighterPreviewLoopP0StatusFinal, gNdsFighterPreviewLoopP1StatusFinal, gNdsFighterPreviewLoopP0MotionFinal, gNdsFighterPreviewLoopP1MotionFinal, gNdsFighterPreviewLoopP0GAFinal, gNdsFighterPreviewLoopP1GAFinal, gNdsFighterPreviewLoopP0StatusVisitMask, gNdsFighterPreviewLoopP1StatusVisitMask, gNdsFighterPreviewLoopP0TransitionMask, gNdsFighterPreviewLoopP1TransitionMask',
        'printf "PREV_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterPreviewLoopP0FrameCount, gNdsFighterPreviewLoopP1FrameCount, gNdsFighterPreviewLoopP0Completed, gNdsFighterPreviewLoopP1Completed, gNdsFighterPreviewLoopP0InterruptCount, gNdsFighterPreviewLoopP1InterruptCount, gNdsFighterPreviewLoopP0PhysicsCount, gNdsFighterPreviewLoopP1PhysicsCount, gNdsFighterPreviewLoopP0MapCount, gNdsFighterPreviewLoopP1MapCount',
        'printf "PREV_MOVE=%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsFighterPreviewLoopP0FloorYMilli, gNdsFighterPreviewLoopP1FloorYMilli, gNdsFighterPreviewLoopP0RootDeltaXMilli, gNdsFighterPreviewLoopP1RootDeltaXMilli, gNdsFighterPreviewLoopP0RootRiseMilli, gNdsFighterPreviewLoopP1RootRiseMilli, gNdsFighterPreviewLoopP0RootYFinalMilli, gNdsFighterPreviewLoopP1RootYFinalMilli, gNdsFighterPreviewLoopP0RootDirectionOK, gNdsFighterPreviewLoopP1RootDirectionOK, gNdsFighterPreviewLoopP0FloorOK, gNdsFighterPreviewLoopP1FloorOK',
        'printf "PREV_DRAW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterPreviewLoopPreviewWidth, gNdsFighterPreviewLoopPreviewHeight, gNdsFighterPreviewLoopPreviewPitch, gNdsFighterPreviewLoopPreviewReady, gNdsFighterPreviewLoopPreviewCommitDelta, gNdsFighterPreviewLoopDrawFrameCount, gNdsFighterPreviewLoopDisplayCallbackCount, gNdsFighterPreviewLoopP0DisplayCallbackCount, gNdsFighterPreviewLoopP1DisplayCallbackCount, gNdsFighterPreviewLoopP0CandidateCount, gNdsFighterPreviewLoopP1CandidateCount, gNdsFighterPreviewLoopP0DrawnDObjCount, gNdsFighterPreviewLoopP1DrawnDObjCount, gNdsFighterPreviewLoopTotalPixelCount, gNdsFighterPreviewLoopP0ColorChecksum, gNdsFighterPreviewLoopP1ColorChecksum',
        'printf "PREV_SCREEN=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsFighterPreviewLoopP0ScreenXStart, gNdsFighterPreviewLoopP1ScreenXStart, gNdsFighterPreviewLoopP0ScreenXFinal, gNdsFighterPreviewLoopP1ScreenXFinal, gNdsFighterPreviewLoopP0ScreenXDelta, gNdsFighterPreviewLoopP1ScreenXDelta, gNdsFighterPreviewLoopP0ScreenYFloor, gNdsFighterPreviewLoopP1ScreenYFloor, gNdsFighterPreviewLoopP0ScreenRise, gNdsFighterPreviewLoopP1ScreenRise',
        'printf "PREV_TRANS=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterPreviewLoopFallDetectCount, gNdsFighterPreviewLoopLandingDetectCount, gNdsFighterPreviewLoopSetGroundCount, gNdsFighterPreviewLoopSetAirCount, gNdsFighterPreviewLoopWaitSetStatusCount, gNdsFighterPreviewLoopRunBrakeEndCount, gNdsFighterPreviewLoopJumpAnimEndCount, gNdsFighterPreviewLoopLandingEndCount, gNdsFighterPreviewLoopDeferredInterruptCheckCount',
        'printf "PREV_SAFE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterPreviewLoopGObjDelta, gNdsFighterPreviewLoopUnexpectedStatusCount, gNdsFighterPreviewLoopDeniedStatusCount, gNdsFighterPreviewLoopProcessAttachEscapeCount, gNdsFighterPreviewLoopDisplayProbeCount, gNdsFighterPreviewLoopGameplayUpdateCount, gNdsFighterPreviewLoopDrawCallCount, gNdsFighterPreviewLoopMatrixCallCount, gNdsFighterPreviewLoopRootYDriftCount, gNdsFighterPreviewLoopGADriftCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $vs = [regex]::Match($gdbStdout, 'VS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $pv = [regex]::Match($gdbStdout, 'PV_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $maps = [regex]::Match($gdbStdout, 'MAPS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $sched = [regex]::Match($gdbStdout, 'SCHED_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $ctrl = [regex]::Match($gdbStdout, 'CTRL_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $loop = [regex]::Match($gdbStdout, 'PREV_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $backend = [regex]::Match($gdbStdout, 'PREV_BACKEND=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $taskman = [regex]::Match($gdbStdout, 'PREV_TASKMAN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $process = [regex]::Match($gdbStdout, 'PREV_PROCESS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'PREV_INPUT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $status = [regex]::Match($gdbStdout, 'PREV_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $calls = [regex]::Match($gdbStdout, 'PREV_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $move = [regex]::Match($gdbStdout, 'PREV_MOVE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $draw = [regex]::Match($gdbStdout, 'PREV_DRAW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $screen = [regex]::Match($gdbStdout, 'PREV_SCREEN=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $trans = [regex]::Match($gdbStdout, 'PREV_TRANS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'PREV_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq $ExpectedMode -and [int]$harn.Groups[3].Value -eq $ExpectedHarnessSceneCurr -and [int]$harn.Groups[4].Value -eq $ExpectedHarnessScenePrev -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) $HarnessSelectMessage $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene is not Pupupu VSBattle from Maps.' $gdbStdout
    if ($ExpectedMode -eq 52) {
        Assert-Condition ($vs.Success -and (Convert-MarkerUInt32 $vs.Groups[1].Value) -eq 0x56535452 -and ((Convert-MarkerUInt32 $vs.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain VS Mode -> PlayersVS transition did not pass.' $gdbStdout
        Assert-Condition ($pv.Success -and (Convert-MarkerUInt32 $pv.Groups[1].Value) -eq 0x50565452 -and ((Convert-MarkerUInt32 $pv.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain PlayersVS -> Maps transition did not pass.' $gdbStdout
        Assert-Condition ($maps.Success -and (Convert-MarkerUInt32 $maps.Groups[1].Value) -eq 0x4d53454c -and ((Convert-MarkerUInt32 $maps.Groups[2].Value) -band 0xff) -eq 0xff -and [int]$maps.Groups[9].Value -eq 6) 'Menu-chain Maps -> VSBattle transition did not pass.' $gdbStdout
    }
    $sch = Get-Ints $sched
    Assert-Condition ($sched.Success -and $sch[0] -eq 0x46534c50 -and $sch[1] -eq 0x46534c53 -and (($sch[2] -band 0x7ff) -eq 0x7ff) -and $sch[3] -eq 0xff -and $sch[4] -eq 2) 'Prerequisite scheduler-loop proof did not pass.' $gdbStdout
    $ct = Get-Ints $ctrl
    Assert-Condition ($ctrl.Success -and $ct[0] -eq 0x46434c50 -and $ct[1] -eq 0x46434c53 -and (($ct[2] -band 0xfff) -eq 0xfff) -and $ct[3] -eq 0xff -and $ct[4] -eq 2) 'Prerequisite controller-loop proof did not pass.' $gdbStdout
    $lp = Get-Ints $loop
    Assert-Condition ($loop.Success -and $lp[0] -eq 0x46504c56 -and $lp[1] -eq 0x46505653 -and (($lp[2] -band 0x1fff) -eq 0x1fff) -and $lp[3] -eq 0xff -and $lp[4] -eq 2 -and $lp[5] -ge 160 -and $lp[6] -ge 200) 'Preview-loop result/mask did not pass.' $gdbStdout
    $be = Get-Ints $backend
    Assert-Condition ($be[0] -eq 1 -and (($be[1] -band 0x3) -eq 0x3) -and $be[2] -gt 0 -and $be[3] -ge $be[5] -and $be[4] -eq 0 -and $be[5] -gt 0 -and $be[6] -eq $be[5]) 'Preview-loop controller playback/read-update path failed.' $gdbStdout
    $tm = Get-Ints $taskman
    Assert-Condition ($tm[0] -eq 1 -and $tm[1] -gt 0 -and $tm[2] -gt 0 -and $tm[3] -eq $tm[2] -and $tm[4] -gt 0 -and $tm[5] -ge $tm[1]) 'Preview-loop taskman/VSBattle update path failed.' $gdbStdout
    $pr = Get-Ints $process
    Assert-Condition ($pr[0] -eq 1 -and $pr[1] -eq 1 -and $pr[2] -gt 0 -and $pr[3] -gt 0 -and $pr[4] -eq $pr[2] -and $pr[5] -eq $pr[3] -and $pr[6] -eq 0) 'Preview-loop GObjProcess attach/run path failed.' $gdbStdout
    $inp = Get-Ints $input
    Assert-Condition ($inp[0] -gt 0 -and $inp[1] -gt 0 -and $inp[2] -gt 0 -and $inp[3] -gt 0 -and $inp[4] -eq 0 -and $inp[5] -eq 0 -and $inp[6] -gt 0 -and $inp[7] -gt 0 -and $inp[8] -ne 0 -and $inp[9] -ne 0 -and $inp[10] -ne 0 -and $inp[11] -ne 0 -and $inp[12] -gt 0 -and $inp[13] -gt 0) 'Preview-loop input bridge failed or wrote FTStruct directly.' $gdbStdout
    $st = Get-Ints $status
    Assert-Condition (($st[0..9] -join ',') -eq '10,10,4,4,10,10,4,4,0,0') 'Preview-loop start/final state was not Wait/Ground.' $gdbStdout
    Assert-Condition ((($st[10] -band 0x3ff) -eq 0x3ff) -and (($st[11] -band 0x3ff) -eq 0x3ff) -and (($st[12] -band 0x7ff) -eq 0x7ff) -and (($st[13] -band 0x7ff) -eq 0x7ff)) 'Preview-loop status/transition masks were incomplete.' $gdbStdout
    $ca = Get-Ints $calls
    Assert-Condition ($ca[2] -eq 1 -and $ca[3] -eq 1 -and $ca[0] -gt 0 -and $ca[1] -gt 0 -and $ca[0] -le 180 -and $ca[1] -le 180 -and (@($ca[4..9] | Where-Object { $_ -le 0 }).Count -eq 0)) 'Preview-loop frame callback counters failed.' $gdbStdout
    $mv = Get-Ints $move
    Assert-Condition ($mv[2] -ne 0 -and $mv[3] -ne 0 -and $mv[4] -gt 0 -and $mv[5] -gt 0 -and $mv[6] -eq $mv[0] -and $mv[7] -eq $mv[1] -and $mv[8] -eq 1 -and $mv[9] -eq 1 -and $mv[10] -eq 1 -and $mv[11] -eq 1) 'Preview-loop movement/floor markers failed.' $gdbStdout
    $dr = Get-Ints $draw
    Assert-Condition ($dr[0] -eq 96 -and $dr[1] -eq 72 -and $dr[2] -ge 96 -and $dr[3] -eq 1 -and $dr[4] -ge 7 -and $dr[5] -ge 7 -and $dr[6] -ge 14 -and $dr[7] -ge 7 -and $dr[8] -ge 7 -and $dr[9] -ge 14 -and $dr[10] -ge 18 -and $dr[11] -ge 14 -and $dr[12] -ge 18 -and $dr[13] -gt 0 -and $dr[14] -ne 0 -and $dr[15] -ne 0) 'Preview-loop draw/DObj/pixel markers failed.' $gdbStdout
    $sc = Get-Ints $screen
    Assert-Condition ($sc[4] -ne 0 -and $sc[5] -ne 0 -and $sc[8] -gt 0 -and $sc[9] -gt 0) 'Preview-loop screen movement markers failed.' $gdbStdout
    $tr = Get-Ints $trans
    Assert-Condition ($tr[0] -ge 2 -and $tr[1] -ge 2 -and $tr[2] -ge 2 -and $tr[3] -ge 2 -and $tr[4] -ge 4 -and $tr[5] -ge 2 -and $tr[6] -ge 2 -and $tr[7] -ge 2 -and $tr[8] -gt 0) 'Preview-loop transition counters failed.' $gdbStdout
    $sf = Get-Ints $safe
    Assert-Condition ((@($sf[0..9] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'Preview-loop safe escape counters were not zero.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after preview-loop proof.' $gdbStdout
    Write-Output ("$Label harness passed: scene=22/21 drawFrames={0} callbacks={1} pixels={2} screenDx={3}/{4} screenRise={5}/{6} rootDx={7}/{8} final=Wait/Ground safe=1" -f $dr[5], $dr[6], $dr[13], $sc[4], $sc[5], $sc[8], $sc[9], $mv[2], $mv[3])
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-GdbMarkerTemps -Root $root -ScriptName $scriptName
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
}
