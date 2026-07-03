param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$ImportBattleShipFTManager,
    [string]$Harness = 'battle_mariofox_gcrunall_loop',
    [string]$Target = 'smash64ds-battle-mariofox-gcrunall-loop',
    [string]$Build = 'build-battle-mariofox-gcrunall-loop-harness',
    [int]$ExpectedMode = 53,
    [int]$ExpectedHarnessSceneCurr = 22,
    [int]$ExpectedHarnessScenePrev = 21,
    [string]$Label = 'Battle Mario/Fox gcRunAll-loop',
    [string]$HarnessSelectMessage = 'Direct gcRunAll-loop harness did not select VSBattle from Maps.'
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ($ImportBattleShipFTManager) {
    if ($Target -notlike '*-ftmanager') {
        $Target = "$Target-ftmanager"
    }
    if ($Build -notlike '*-ftmanager-harness') {
        $Build = $Build -replace '-harness$', '-ftmanager-harness'
    }
}
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
$makeArgs = @('-C', $root, "TARGET=$Target", "BUILD=$Build", "NDS_DEV_SCENE_HARNESS=$Harness", '-j16')
if ($ImportBattleShipFTManager) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FTMANAGER=1'
}
& make @makeArgs
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
        'printf "VS_TRANS=%#x,%#x\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask',
        'printf "PV_TRANS=%#x,%#x\n", gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask',
        'printf "MAPS_TRANS=%#x,%#x,%u\n", gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask, gNdsMapsSelectTransitionSelectedGKind',
        'printf "PREV_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxPreviewLoopResult, gNdsFighterMarioFoxPreviewLoopSafeResult, gNdsFighterMarioFoxPreviewLoopMask, gNdsFighterMarioFoxPreviewLoopDeferredMask, gNdsFighterMarioFoxPreviewLoopCount',
        'printf "GCRUNALL_LOOP=%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxGCRunAllLoopResult, gNdsFighterMarioFoxGCRunAllLoopSafeResult, gNdsFighterMarioFoxGCRunAllLoopMask, gNdsFighterMarioFoxGCRunAllLoopDeferredMask, gNdsFighterMarioFoxGCRunAllLoopCount, gNdsFighterGCRunAllLoopFrameMax, gNdsFighterGCRunAllLoopUpdateMax',
        'printf "GCRUNALL_TASKMAN=%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopPrepared, gNdsFighterGCRunAllLoopTaskmanUpdateCount, gNdsFighterGCRunAllLoopVSBattleUpdateCount, gNdsFighterGCRunAllLoopBaseVSBattleUpdateCount, gNdsFighterGCRunAllLoopRunAllCount, gNdsTaskmanBoundedUpdateCount',
        'printf "NAT_MOTION=%#x,%#x,%#x,%u,%u,%u,%u,%u,%#x,%u,%u\n", gNdsFighterNaturalMotionResult, gNdsFighterNaturalMotionSafeResult, gNdsFighterNaturalMotionMask, gNdsFighterNaturalMotionPrepared, gNdsFighterNaturalMotionUpdateCount, gNdsFighterNaturalMotionBaseVSBattleUpdateCount, gNdsFighterNaturalMotionRunAllCount, gNdsFighterNaturalMotionControllerReadCount, gNdsFighterNaturalMotionManagerMask, gNdsFighterNaturalMotionGObjDelta, gNdsFighterNaturalMotionUnsafeCount',
        'printf "NAT_FIG=%u,%u,%u,%u\n", gNdsFighterNaturalMotionFigatreeAttachCount, gNdsFighterNaturalMotionFigatreeNullCount, gNdsFighterNaturalMotionFigatreeTableInvalidCount, gNdsFighterNaturalMotionFigatreeAnimInvalidCount',
        'printf "NAT_WAIT=%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterNaturalMotionP0WaitFrameCount, gNdsFighterNaturalMotionP1WaitFrameCount, gNdsFighterNaturalMotionP0AnimAdvanceCount, gNdsFighterNaturalMotionP1AnimAdvanceCount, gNdsFighterNaturalMotionP0ValidJointCount, gNdsFighterNaturalMotionP1ValidJointCount, gNdsFighterNaturalMotionP0AnimStartBits, gNdsFighterNaturalMotionP1AnimStartBits, gNdsFighterNaturalMotionP0AnimFinalBits, gNdsFighterNaturalMotionP1AnimFinalBits',
        'printf "NAT_WALK=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterNaturalMotionWalkInputFrame, gNdsFighterNaturalMotionP0WalkFrameCount, gNdsFighterNaturalMotionP1WalkFrameCount, gNdsFighterNaturalMotionP0StatusStart, gNdsFighterNaturalMotionP1StatusStart, gNdsFighterNaturalMotionP0StatusFinal, gNdsFighterNaturalMotionP1StatusFinal, gNdsFighterNaturalMotionP0WalkStatus, gNdsFighterNaturalMotionP1WalkStatus, gNdsFighterNaturalMotionP0WalkMotion, gNdsFighterNaturalMotionP1WalkMotion',
        'printf "GCRUNALL_RUN=%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopOldProcessPauseCount, gNdsFighterGCRunAllLoopNonTargetGObjVisitCount, gNdsFighterGCRunAllLoopNonTargetProcessPauseCount, gNdsFighterGCRunAllLoopTargetProcessPreserveCount, gNdsFighterGCRunAllLoopGObjCountBefore, gNdsFighterGCRunAllLoopGObjCountAfter',
        'printf "GCRUNALL_PROCESS=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0ProcessAttachCount, gNdsFighterGCRunAllLoopP1ProcessAttachCount, gNdsFighterGCRunAllLoopP0GObjProcessRunCount, gNdsFighterGCRunAllLoopP1GObjProcessRunCount, gNdsFighterGCRunAllLoopP0ProcCallbackCount, gNdsFighterGCRunAllLoopP1ProcCallbackCount, gNdsFighterGCRunAllLoopProcessAttachEscapeCount',
        'printf "GCRUNALL_INPUT=%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterGCRunAllLoopP0PlaybackApplyCount, gNdsFighterGCRunAllLoopP1PlaybackApplyCount, gNdsFighterGCRunAllLoopP0ControllerToFTInputCount, gNdsFighterGCRunAllLoopP1ControllerToFTInputCount, gNdsFighterGCRunAllLoopP0DirectFTInputWriteCount, gNdsFighterGCRunAllLoopP1DirectFTInputWriteCount, gNdsFighterGCRunAllLoopP0DashTapEligibleCount, gNdsFighterGCRunAllLoopP1DashTapEligibleCount, gNdsFighterGCRunAllLoopP0ButtonTapMask, gNdsFighterGCRunAllLoopP1ButtonTapMask, gNdsFighterGCRunAllLoopP0ButtonHoldMask, gNdsFighterGCRunAllLoopP1ButtonHoldMask, gNdsFighterGCRunAllLoopP0JumpButtonTapCount, gNdsFighterGCRunAllLoopP1JumpButtonTapCount',
        'printf "GCRUNALL_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterGCRunAllLoopP0StatusStart, gNdsFighterGCRunAllLoopP1StatusStart, gNdsFighterGCRunAllLoopP0MotionStart, gNdsFighterGCRunAllLoopP1MotionStart, gNdsFighterGCRunAllLoopP0StatusFinal, gNdsFighterGCRunAllLoopP1StatusFinal, gNdsFighterGCRunAllLoopP0MotionFinal, gNdsFighterGCRunAllLoopP1MotionFinal, gNdsFighterGCRunAllLoopP0GAFinal, gNdsFighterGCRunAllLoopP1GAFinal, gNdsFighterGCRunAllLoopP0StatusVisitMask, gNdsFighterGCRunAllLoopP1StatusVisitMask, gNdsFighterGCRunAllLoopP0TransitionMask, gNdsFighterGCRunAllLoopP1TransitionMask',
        'printf "GCRUNALL_VISITS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0WaitVisitCount, gNdsFighterGCRunAllLoopP1WaitVisitCount, gNdsFighterGCRunAllLoopP0WalkVisitCount, gNdsFighterGCRunAllLoopP1WalkVisitCount, gNdsFighterGCRunAllLoopP0DashVisitCount, gNdsFighterGCRunAllLoopP1DashVisitCount, gNdsFighterGCRunAllLoopP0RunVisitCount, gNdsFighterGCRunAllLoopP1RunVisitCount, gNdsFighterGCRunAllLoopP0RunBrakeVisitCount, gNdsFighterGCRunAllLoopP1RunBrakeVisitCount, gNdsFighterGCRunAllLoopP0KneeBendVisitCount, gNdsFighterGCRunAllLoopP1KneeBendVisitCount, gNdsFighterGCRunAllLoopP0JumpVisitCount, gNdsFighterGCRunAllLoopP1JumpVisitCount, gNdsFighterGCRunAllLoopP0FallVisitCount, gNdsFighterGCRunAllLoopP1FallVisitCount, gNdsFighterGCRunAllLoopP0LandingVisitCount, gNdsFighterGCRunAllLoopP1LandingVisitCount',
        'printf "GCRUNALL_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0FrameCount, gNdsFighterGCRunAllLoopP1FrameCount, gNdsFighterGCRunAllLoopP0Completed, gNdsFighterGCRunAllLoopP1Completed, gNdsFighterGCRunAllLoopP0InterruptCount, gNdsFighterGCRunAllLoopP1InterruptCount, gNdsFighterGCRunAllLoopP0PhysicsCount, gNdsFighterGCRunAllLoopP1PhysicsCount, gNdsFighterGCRunAllLoopP0MapCount, gNdsFighterGCRunAllLoopP1MapCount',
        'printf "GCRUNALL_DRAW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterGCRunAllLoopPreviewWidth, gNdsFighterGCRunAllLoopPreviewHeight, gNdsFighterGCRunAllLoopPreviewPitch, gNdsFighterGCRunAllLoopPreviewReady, gNdsFighterGCRunAllLoopPreviewCommitDelta, gNdsFighterGCRunAllLoopDrawFrameCount, gNdsFighterGCRunAllLoopDisplayCallbackCount, gNdsFighterGCRunAllLoopP0DisplayCallbackCount, gNdsFighterGCRunAllLoopP1DisplayCallbackCount, gNdsFighterGCRunAllLoopP0CandidateCount, gNdsFighterGCRunAllLoopP1CandidateCount, gNdsFighterGCRunAllLoopP0DrawnDObjCount, gNdsFighterGCRunAllLoopP1DrawnDObjCount, gNdsFighterGCRunAllLoopTotalPixelCount, gNdsFighterGCRunAllLoopP0ColorChecksum, gNdsFighterGCRunAllLoopP1ColorChecksum',
        'printf "GCRUNALL_SCREEN=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsFighterGCRunAllLoopP0ScreenXStart, gNdsFighterGCRunAllLoopP1ScreenXStart, gNdsFighterGCRunAllLoopP0ScreenXFinal, gNdsFighterGCRunAllLoopP1ScreenXFinal, gNdsFighterGCRunAllLoopP0ScreenXDelta, gNdsFighterGCRunAllLoopP1ScreenXDelta, gNdsFighterGCRunAllLoopP0ScreenYFloor, gNdsFighterGCRunAllLoopP1ScreenYFloor, gNdsFighterGCRunAllLoopP0ScreenRise, gNdsFighterGCRunAllLoopP1ScreenRise',
        'printf "GCRUNALL_MOVE=%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopP0FloorYMilli, gNdsFighterGCRunAllLoopP1FloorYMilli, gNdsFighterGCRunAllLoopP0RootDeltaXMilli, gNdsFighterGCRunAllLoopP1RootDeltaXMilli, gNdsFighterGCRunAllLoopP0RootRiseMilli, gNdsFighterGCRunAllLoopP1RootRiseMilli, gNdsFighterGCRunAllLoopP0RootYFinalMilli, gNdsFighterGCRunAllLoopP1RootYFinalMilli, gNdsFighterGCRunAllLoopP0RootDirectionOK, gNdsFighterGCRunAllLoopP1RootDirectionOK, gNdsFighterGCRunAllLoopP0FloorOK, gNdsFighterGCRunAllLoopP1FloorOK',
        'printf "GCRUNALL_TRANS=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopFallDetectCount, gNdsFighterGCRunAllLoopLandingDetectCount, gNdsFighterGCRunAllLoopSetGroundCount, gNdsFighterGCRunAllLoopSetAirCount, gNdsFighterGCRunAllLoopWaitSetStatusCount, gNdsFighterGCRunAllLoopRunBrakeEndCount, gNdsFighterGCRunAllLoopJumpAnimEndCount, gNdsFighterGCRunAllLoopLandingEndCount, gNdsFighterGCRunAllLoopDeferredInterruptCheckCount',
        'printf "GCRUNALL_SAFE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCRunAllLoopGObjDelta, gNdsFighterGCRunAllLoopUnexpectedStatusCount, gNdsFighterGCRunAllLoopDeniedStatusCount, gNdsFighterGCRunAllLoopProcessAttachEscapeCount, gNdsFighterGCRunAllLoopDisplayProbeCount, gNdsFighterGCRunAllLoopGameplayUpdateCount, gNdsFighterGCRunAllLoopDrawCallCount, gNdsFighterGCRunAllLoopMatrixCallCount, gNdsFighterGCRunAllLoopRootYDriftCount, gNdsFighterGCRunAllLoopGADriftCount',
        'printf "PLATFORM_DL_PREVIEW=%u,%u,%u,%u\n", gNdsOriginalDLPreviewReady, gNdsOriginalDLPreviewWidth, gNdsOriginalDLPreviewHeight, gNdsOriginalDLPreviewCommitCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $vs = [regex]::Match($gdbStdout, 'VS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $pv = [regex]::Match($gdbStdout, 'PV_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $maps = [regex]::Match($gdbStdout, 'MAPS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $prev = [regex]::Match($gdbStdout, 'PREV_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $loop = [regex]::Match($gdbStdout, 'GCRUNALL_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $taskman = [regex]::Match($gdbStdout, 'GCRUNALL_TASKMAN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $natural = [regex]::Match($gdbStdout, 'NAT_MOTION=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $naturalFig = [regex]::Match($gdbStdout, 'NAT_FIG=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalWait = [regex]::Match($gdbStdout, 'NAT_WAIT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $naturalWalk = [regex]::Match($gdbStdout, 'NAT_WALK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $run = [regex]::Match($gdbStdout, 'GCRUNALL_RUN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $process = [regex]::Match($gdbStdout, 'GCRUNALL_PROCESS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'GCRUNALL_INPUT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $status = [regex]::Match($gdbStdout, 'GCRUNALL_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $visits = [regex]::Match($gdbStdout, 'GCRUNALL_VISITS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $calls = [regex]::Match($gdbStdout, 'GCRUNALL_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $draw = [regex]::Match($gdbStdout, 'GCRUNALL_DRAW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $screen = [regex]::Match($gdbStdout, 'GCRUNALL_SCREEN=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $move = [regex]::Match($gdbStdout, 'GCRUNALL_MOVE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $trans = [regex]::Match($gdbStdout, 'GCRUNALL_TRANS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'GCRUNALL_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platform = [regex]::Match($gdbStdout, 'PLATFORM_DL_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq $ExpectedMode -and [int]$harn.Groups[3].Value -eq $ExpectedHarnessSceneCurr -and [int]$harn.Groups[4].Value -eq $ExpectedHarnessScenePrev -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) $HarnessSelectMessage $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene is not Pupupu VSBattle from Maps.' $gdbStdout
    if ($ExpectedMode -eq 54) {
        Assert-Condition ($vs.Success -and (Convert-MarkerUInt32 $vs.Groups[1].Value) -eq 0x56535452 -and ((Convert-MarkerUInt32 $vs.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain VS Mode -> PlayersVS transition did not pass.' $gdbStdout
        Assert-Condition ($pv.Success -and (Convert-MarkerUInt32 $pv.Groups[1].Value) -eq 0x50565452 -and ((Convert-MarkerUInt32 $pv.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain PlayersVS -> Maps transition did not pass.' $gdbStdout
        Assert-Condition ($maps.Success -and (Convert-MarkerUInt32 $maps.Groups[1].Value) -eq 0x4d53454c -and ((Convert-MarkerUInt32 $maps.Groups[2].Value) -band 0xff) -eq 0xff -and [int]$maps.Groups[3].Value -eq 6) 'Menu-chain Maps -> VSBattle transition did not pass.' $gdbStdout
    }
    if ($ImportBattleShipFTManager) {
        $nat = Get-Ints $natural
        $nfig = Get-Ints $naturalFig
        $nw = Get-Ints $naturalWait
        $nwalk = Get-Ints $naturalWalk
        Assert-Condition ($natural.Success -and $nat[0] -eq 0x464e4d50 -and $nat[1] -eq 0x464e4d53 -and (($nat[2] -band 0x3ff) -eq 0x3ff) -and $nat[3] -eq 1 -and $nat[4] -gt 0 -and $nat[5] -gt 0 -and $nat[6] -gt 0 -and $nat[7] -gt 0 -and (($nat[8] -band 0x3) -eq 0x3) -and $nat[10] -eq 0) 'Natural-motion manager runtime proof failed.' $gdbStdout
        Assert-Condition ($naturalFig.Success -and $nfig[0] -gt 0 -and $nfig[2] -eq 0 -and $nfig[3] -eq 0) 'Natural-motion figatree attach proof failed.' $gdbStdout
        Assert-Condition ($naturalWait.Success -and $nw[0] -ge 300 -and $nw[1] -ge 300 -and $nw[2] -gt 0 -and $nw[3] -gt 0 -and $nw[4] -ge 300 -and $nw[5] -ge 300 -and $nw[6] -ne $nw[8] -and $nw[7] -ne $nw[9]) 'Natural-motion Wait animation proof failed.' $gdbStdout
        Assert-Condition ($naturalWalk.Success -and $nwalk[0] -gt 0 -and $nwalk[1] -ge 8 -and $nwalk[2] -ge 8 -and $nwalk[7] -gt 0 -and $nwalk[8] -gt 0 -and $nwalk[9] -gt 0 -and $nwalk[10] -gt 0) 'Natural-motion Walk transition proof failed.' $gdbStdout
        Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after natural-motion proof.' $gdbStdout
        Write-Output ("$Label ftmanager natural-motion harness passed: wait={0}/{1} anim={2}/{3} walk={4}/{5} updates={6} mask=0x{7:x}" -f $nw[0], $nw[1], $nw[2], $nw[3], $nwalk[1], $nwalk[2], $nat[4], $nat[2])
        return
    }
    $prv = Get-Ints $prev
    Assert-Condition ($prev.Success -and $prv[0] -eq 0x46504c56 -and $prv[1] -eq 0x46505653 -and (($prv[2] -band 0x1fff) -eq 0x1fff) -and $prv[3] -eq 0xff -and $prv[4] -eq 2) 'Prerequisite preview-loop proof did not pass.' $gdbStdout
    $lp = Get-Ints $loop
    Assert-Condition ($loop.Success -and $lp[0] -eq 0x4647414c -and $lp[1] -eq 0x46474153 -and (($lp[2] -band 0x1fff) -eq 0x1fff) -and $lp[3] -eq 0xff -and $lp[4] -eq 2 -and $lp[5] -ge 160 -and $lp[6] -ge 220) 'gcRunAll-loop result/mask did not pass.' $gdbStdout
    $tm = Get-Ints $taskman
    Assert-Condition ($tm[0] -eq 1 -and $tm[1] -gt 0 -and $tm[2] -gt 0 -and $tm[3] -eq $tm[2] -and $tm[4] -gt 0 -and $tm[5] -ge $tm[1]) 'gcRunAll-loop taskman path failed.' $gdbStdout
    $rn = Get-Ints $run
    Assert-Condition ($rn[0] -gt 0 -and $rn[1] -gt 0 -and $rn[3] -ge 2 -and $rn[4] -eq $rn[5]) 'gcRunAll pause/preserve/GObj stability setup failed.' $gdbStdout
    $pc = Get-Ints $process
    Assert-Condition ($pc[0] -eq 1 -and $pc[1] -eq 1 -and $pc[2] -gt 0 -and $pc[3] -gt 0 -and $pc[4] -eq $pc[2] -and $pc[5] -eq $pc[3] -and $pc[6] -eq 0) 'gcRunAll process attach/callback path failed.' $gdbStdout
    $inp = Get-Ints $input
    Assert-Condition ($inp[0] -gt 0 -and $inp[1] -gt 0 -and $inp[2] -gt 0 -and $inp[3] -gt 0 -and $inp[4] -eq 0 -and $inp[5] -eq 0 -and $inp[6] -gt 0 -and $inp[7] -gt 0 -and $inp[8] -ne 0 -and $inp[9] -ne 0 -and $inp[10] -ne 0 -and $inp[11] -ne 0 -and $inp[12] -gt 0 -and $inp[13] -gt 0) 'gcRunAll input bridge failed or wrote FTStruct directly.' $gdbStdout
    $st = Get-Ints $status
    Assert-Condition (($st[0..9] -join ',') -eq '10,10,4,4,10,10,4,4,0,0') 'gcRunAll start/final state was not Wait/Ground.' $gdbStdout
    Assert-Condition ((($st[10] -band 0x3ff) -eq 0x3ff) -and (($st[11] -band 0x3ff) -eq 0x3ff) -and (($st[12] -band 0x7ff) -eq 0x7ff) -and (($st[13] -band 0x7ff) -eq 0x7ff)) 'gcRunAll status/transition masks were incomplete.' $gdbStdout
    $vi = Get-Ints $visits
    Assert-Condition ((@($vi | Where-Object { $_ -le 0 }).Count -eq 0)) 'gcRunAll state visit counters were incomplete.' $gdbStdout
    $ca = Get-Ints $calls
    Assert-Condition ($ca[2] -eq 1 -and $ca[3] -eq 1 -and $ca[0] -gt 0 -and $ca[1] -gt 0 -and $ca[0] -le 180 -and $ca[1] -le 180 -and (@($ca[4..9] | Where-Object { $_ -le 0 }).Count -eq 0)) 'gcRunAll frame callback counters failed.' $gdbStdout
    $dr = Get-Ints $draw
    Assert-Condition ($dr[0] -eq 96 -and $dr[1] -eq 72 -and $dr[2] -ge 96 -and $dr[3] -eq 1 -and $dr[4] -ge 7 -and $dr[5] -ge 7 -and $dr[6] -ge 14 -and $dr[7] -ge 7 -and $dr[8] -ge 7 -and $dr[9] -ge 14 -and $dr[10] -ge 18 -and $dr[11] -ge 14 -and $dr[12] -ge 18 -and $dr[13] -gt 0 -and $dr[14] -ne 0 -and $dr[15] -ne 0) 'gcRunAll draw/DObj/pixel markers failed.' $gdbStdout
    $sc = Get-Ints $screen
    Assert-Condition ($sc[4] -ne 0 -and $sc[5] -ne 0 -and $sc[8] -gt 0 -and $sc[9] -gt 0) 'gcRunAll screen movement markers failed.' $gdbStdout
    $mv = Get-Ints $move
    Assert-Condition ($mv[2] -ne 0 -and $mv[3] -ne 0 -and $mv[4] -gt 0 -and $mv[5] -gt 0 -and $mv[6] -eq $mv[0] -and $mv[7] -eq $mv[1] -and $mv[8] -eq 1 -and $mv[9] -eq 1 -and $mv[10] -eq 1 -and $mv[11] -eq 1) 'gcRunAll movement/floor markers failed.' $gdbStdout
    $tr = Get-Ints $trans
    Assert-Condition ($tr[0] -ge 2 -and $tr[1] -ge 2 -and $tr[2] -ge 2 -and $tr[3] -ge 2 -and $tr[4] -ge 4 -and $tr[5] -ge 2 -and $tr[6] -ge 2 -and $tr[7] -ge 2 -and $tr[8] -gt 0) 'gcRunAll transition counters failed.' $gdbStdout
    $sf = Get-Ints $safe
    Assert-Condition ((@($sf[0..9] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'gcRunAll safe escape counters were not zero.' $gdbStdout
    $pd = Get-Ints $platform
    Assert-Condition ($pd[0] -eq 1 -and $pd[1] -eq 96 -and $pd[2] -eq 72 -and $pd[3] -ge $dr[4]) 'Platform original-DL preview markers failed.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after gcRunAll proof.' $gdbStdout
    Write-Output ("$Label harness passed: scene=22/21 gcRunAll={0} callbacks={1}/{2} draws={3} pixels={4} visits=0x{5:x}/0x{6:x} transitions=0x{7:x}/0x{8:x} screen-dx={9}/{10} screen-rise={11}/{12} final=Wait/Ground safe=1" -f $tm[4], $pc[4], $pc[5], $dr[5], $dr[13], $st[10], $st[11], $st[12], $st[13], $sc[4], $sc[5], $sc[8], $sc[9])
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
