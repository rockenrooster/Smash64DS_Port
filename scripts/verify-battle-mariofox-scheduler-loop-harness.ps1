param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [string]$Harness = 'battle_mariofox_scheduler_loop',
    [string]$Target = 'smash64ds-battle-mariofox-scheduler-loop',
    [string]$Build = 'build-battle-mariofox-scheduler-loop-harness',
    [int]$ExpectedMode = 47,
    [int]$ExpectedHarnessSceneCurr = 22,
    [int]$ExpectedHarnessScenePrev = 21,
    [string]$Label = 'Battle Mario/Fox scheduler-loop',
    [string]$HarnessSelectMessage = 'Direct scheduler-loop harness did not select VSBattle from Maps.'
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
        'printf "PROC_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxProcessLoopResult, gNdsFighterMarioFoxProcessLoopSafeResult, gNdsFighterMarioFoxProcessLoopMask, gNdsFighterMarioFoxProcessLoopDeferredMask, gNdsFighterMarioFoxProcessLoopCount',
        'printf "PROC_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterProcessLoopP0StatusStart, gNdsFighterProcessLoopP1StatusStart, gNdsFighterProcessLoopP0MotionStart, gNdsFighterProcessLoopP1MotionStart, gNdsFighterProcessLoopP0StatusFinal, gNdsFighterProcessLoopP1StatusFinal, gNdsFighterProcessLoopP0MotionFinal, gNdsFighterProcessLoopP1MotionFinal, gNdsFighterProcessLoopP0GAFinal, gNdsFighterProcessLoopP1GAFinal',
        'printf "SCHED_LOOP=%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxSchedulerLoopResult, gNdsFighterMarioFoxSchedulerLoopSafeResult, gNdsFighterMarioFoxSchedulerLoopMask, gNdsFighterMarioFoxSchedulerLoopDeferredMask, gNdsFighterMarioFoxSchedulerLoopCount, gNdsFighterSchedulerLoopFrameMax, gNdsFighterSchedulerLoopUpdateMax',
        'printf "SCHED_TASKMAN=%u,%u,%u,%u,%u\n", gNdsFighterSchedulerLoopPrepared, gNdsFighterSchedulerLoopTaskmanUpdateCount, gNdsFighterSchedulerLoopVSBattleUpdateCount, gNdsFighterSchedulerLoopBaseVSBattleUpdateCount, gNdsFighterSchedulerLoopSchedulerUpdateCount',
        'printf "SCHED_PROCESS=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterSchedulerLoopP0ProcessAttachCount, gNdsFighterSchedulerLoopP1ProcessAttachCount, gNdsFighterSchedulerLoopP0GObjProcessRunCount, gNdsFighterSchedulerLoopP1GObjProcessRunCount, gNdsFighterSchedulerLoopP0ProcCallbackCount, gNdsFighterSchedulerLoopP1ProcCallbackCount, gNdsFighterSchedulerLoopProcessAttachEscapeCount',
        'printf "SCHED_INPUT=%u,%u,%u,%u,%#x,%#x,%d,%d\n", gNdsFighterSchedulerLoopP0InputApplyCount, gNdsFighterSchedulerLoopP1InputApplyCount, gNdsFighterSchedulerLoopControllerBridgeCount, gNdsFighterSchedulerLoopControllerMirrorCount, gNdsFighterSchedulerLoopP0ButtonTapMask, gNdsFighterSchedulerLoopP1ButtonTapMask, gNdsFighterSchedulerLoopP0LastStickX, gNdsFighterSchedulerLoopP1LastStickX',
        'printf "SCHED_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterSchedulerLoopP0StatusStart, gNdsFighterSchedulerLoopP1StatusStart, gNdsFighterSchedulerLoopP0MotionStart, gNdsFighterSchedulerLoopP1MotionStart, gNdsFighterSchedulerLoopP0StatusFinal, gNdsFighterSchedulerLoopP1StatusFinal, gNdsFighterSchedulerLoopP0MotionFinal, gNdsFighterSchedulerLoopP1MotionFinal, gNdsFighterSchedulerLoopP0GAFinal, gNdsFighterSchedulerLoopP1GAFinal, gNdsFighterSchedulerLoopP0StatusVisitMask, gNdsFighterSchedulerLoopP1StatusVisitMask, gNdsFighterSchedulerLoopP0TransitionMask, gNdsFighterSchedulerLoopP1TransitionMask',
        'printf "SCHED_VISITS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterSchedulerLoopP0FrameCount, gNdsFighterSchedulerLoopP1FrameCount, gNdsFighterSchedulerLoopP0Completed, gNdsFighterSchedulerLoopP1Completed, gNdsFighterSchedulerLoopP0WaitVisitCount, gNdsFighterSchedulerLoopP1WaitVisitCount, gNdsFighterSchedulerLoopP0WalkVisitCount, gNdsFighterSchedulerLoopP1WalkVisitCount, gNdsFighterSchedulerLoopP0DashVisitCount, gNdsFighterSchedulerLoopP1DashVisitCount, gNdsFighterSchedulerLoopP0RunVisitCount, gNdsFighterSchedulerLoopP1RunVisitCount, gNdsFighterSchedulerLoopP0RunBrakeVisitCount, gNdsFighterSchedulerLoopP1RunBrakeVisitCount, gNdsFighterSchedulerLoopP0KneeBendVisitCount, gNdsFighterSchedulerLoopP1KneeBendVisitCount, gNdsFighterSchedulerLoopP0JumpVisitCount, gNdsFighterSchedulerLoopP1JumpVisitCount, gNdsFighterSchedulerLoopP0FallVisitCount, gNdsFighterSchedulerLoopP1FallVisitCount, gNdsFighterSchedulerLoopP0LandingVisitCount, gNdsFighterSchedulerLoopP1LandingVisitCount',
        'printf "SCHED_CALLS=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterSchedulerLoopP0InterruptCount, gNdsFighterSchedulerLoopP1InterruptCount, gNdsFighterSchedulerLoopP0PhysicsCount, gNdsFighterSchedulerLoopP1PhysicsCount, gNdsFighterSchedulerLoopP0IntegrateCount, gNdsFighterSchedulerLoopP1IntegrateCount, gNdsFighterSchedulerLoopP0MapCount, gNdsFighterSchedulerLoopP1MapCount',
        'printf "SCHED_MOVE=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsFighterSchedulerLoopP0FloorYMilli, gNdsFighterSchedulerLoopP1FloorYMilli, gNdsFighterSchedulerLoopP0RootXStartMilli, gNdsFighterSchedulerLoopP1RootXStartMilli, gNdsFighterSchedulerLoopP0RootXFinalMilli, gNdsFighterSchedulerLoopP1RootXFinalMilli, gNdsFighterSchedulerLoopP0RootDeltaXMilli, gNdsFighterSchedulerLoopP1RootDeltaXMilli, gNdsFighterSchedulerLoopP0RootRiseMilli, gNdsFighterSchedulerLoopP1RootRiseMilli, gNdsFighterSchedulerLoopP0RootDirectionOK, gNdsFighterSchedulerLoopP1RootDirectionOK, gNdsFighterSchedulerLoopP0FloorOK, gNdsFighterSchedulerLoopP1FloorOK',
        'printf "SCHED_TRANS=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterSchedulerLoopFallDetectCount, gNdsFighterSchedulerLoopLandingDetectCount, gNdsFighterSchedulerLoopSetGroundCount, gNdsFighterSchedulerLoopSetAirCount, gNdsFighterSchedulerLoopWaitSetStatusCount, gNdsFighterSchedulerLoopRunBrakeEndCount, gNdsFighterSchedulerLoopJumpAnimEndCount, gNdsFighterSchedulerLoopLandingEndCount, gNdsFighterSchedulerLoopDeferredInterruptCheckCount',
        'printf "SCHED_SAFE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterSchedulerLoopGObjDelta, gNdsFighterSchedulerLoopUnexpectedStatusCount, gNdsFighterSchedulerLoopDeniedStatusCount, gNdsFighterSchedulerLoopProcessAttachEscapeCount, gNdsFighterSchedulerLoopDisplayProbeCount, gNdsFighterSchedulerLoopGameplayUpdateCount, gNdsFighterSchedulerLoopDrawCallCount, gNdsFighterSchedulerLoopMatrixCallCount, gNdsFighterSchedulerLoopRootYDriftCount, gNdsFighterSchedulerLoopGADriftCount',
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
    $proc = [regex]::Match($gdbStdout, 'PROC_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $procStatus = [regex]::Match($gdbStdout, 'PROC_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $loop = [regex]::Match($gdbStdout, 'SCHED_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $taskman = [regex]::Match($gdbStdout, 'SCHED_TASKMAN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $process = [regex]::Match($gdbStdout, 'SCHED_PROCESS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'SCHED_INPUT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+)')
    $status = [regex]::Match($gdbStdout, 'SCHED_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $visits = [regex]::Match($gdbStdout, 'SCHED_VISITS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $calls = [regex]::Match($gdbStdout, 'SCHED_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $move = [regex]::Match($gdbStdout, 'SCHED_MOVE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $trans = [regex]::Match($gdbStdout, 'SCHED_TRANS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'SCHED_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq $ExpectedMode -and [int]$harn.Groups[3].Value -eq $ExpectedHarnessSceneCurr -and [int]$harn.Groups[4].Value -eq $ExpectedHarnessScenePrev -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) $HarnessSelectMessage $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene is not Pupupu VSBattle from Maps.' $gdbStdout
    if ($ExpectedMode -eq 48) {
        Assert-Condition ($vs.Success -and (Convert-MarkerUInt32 $vs.Groups[1].Value) -eq 0x56535452 -and ((Convert-MarkerUInt32 $vs.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain VS Mode -> PlayersVS transition did not pass.' $gdbStdout
        Assert-Condition ($pv.Success -and (Convert-MarkerUInt32 $pv.Groups[1].Value) -eq 0x50565452 -and ((Convert-MarkerUInt32 $pv.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain PlayersVS -> Maps transition did not pass.' $gdbStdout
        Assert-Condition ($maps.Success -and (Convert-MarkerUInt32 $maps.Groups[1].Value) -eq 0x4d53454c -and ((Convert-MarkerUInt32 $maps.Groups[2].Value) -band 0xff) -eq 0xff -and [int]$maps.Groups[9].Value -eq 6) 'Menu-chain Maps -> VSBattle transition did not pass.' $gdbStdout
    }
    Assert-Condition ($proc.Success -and (Convert-MarkerUInt32 $proc.Groups[1].Value) -eq 0x46504c50 -and (Convert-MarkerUInt32 $proc.Groups[2].Value) -eq 0x46504c53 -and ((Convert-MarkerUInt32 $proc.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $proc.Groups[4].Value) -eq 0xff -and [int]$proc.Groups[5].Value -eq 2) 'Prerequisite process-loop proof did not pass.' $gdbStdout
    $ps = Get-Ints $procStatus
    Assert-Condition (($ps -join ',') -eq '10,10,4,4,10,10,4,4,0,0') 'Prerequisite process-loop final state was not restored.' $gdbStdout
    $lp = Get-Ints $loop
    Assert-Condition ($loop.Success -and $lp[0] -eq 0x46534c50 -and $lp[1] -eq 0x46534c53 -and (($lp[2] -band 0x7ff) -eq 0x7ff) -and $lp[3] -eq 0xff -and $lp[4] -eq 2 -and $lp[5] -ge 120 -and $lp[6] -ge 120) 'Scheduler-loop result/mask did not pass.' $gdbStdout
    $tm = Get-Ints $taskman
    Assert-Condition ($tm[0] -eq 1 -and $tm[1] -gt 0 -and $tm[2] -gt 0 -and $tm[3] -eq $tm[2] -and $tm[4] -gt 0) 'Scheduler-loop taskman/VSBattle update path failed.' $gdbStdout
    $pr = Get-Ints $process
    Assert-Condition ($pr[0] -eq 1 -and $pr[1] -eq 1 -and $pr[2] -gt 0 -and $pr[3] -gt 0 -and $pr[4] -eq $pr[2] -and $pr[5] -eq $pr[3] -and $pr[6] -eq 0) 'Scheduler-loop GObjProcess attach/run path failed.' $gdbStdout
    $inp = Get-Ints $input
    Assert-Condition ($inp[0] -gt 0 -and $inp[1] -gt 0 -and $inp[2] -ge ($inp[0] + $inp[1]) -and $inp[3] -gt 0 -and $inp[4] -ne 0 -and $inp[5] -ne 0) 'Scheduler-loop scripted input bridge failed.' $gdbStdout
    $st = Get-Ints $status
    Assert-Condition (($st[0..9] -join ',') -eq '10,10,4,4,10,10,4,4,0,0') 'Scheduler-loop start/final state was not Wait/Ground.' $gdbStdout
    Assert-Condition ((($st[10] -band 0x3ff) -eq 0x3ff) -and (($st[11] -band 0x3ff) -eq 0x3ff) -and (($st[12] -band 0x7ff) -eq 0x7ff) -and (($st[13] -band 0x7ff) -eq 0x7ff)) 'Scheduler-loop status/transition masks were incomplete.' $gdbStdout
    $v = Get-Ints $visits
    Assert-Condition ($v[2] -eq 1 -and $v[3] -eq 1 -and $v[0] -gt 0 -and $v[1] -gt 0 -and $v[0] -le 160 -and $v[1] -le 160) 'Scheduler-loop did not complete within frame cap.' $gdbStdout
    Assert-Condition ((@($v[4..21] | Where-Object { $_ -le 0 }).Count -eq 0)) 'Scheduler-loop did not visit every expected movement state.' $gdbStdout
    $c = Get-Ints $calls
    Assert-Condition ((@($c[0..7] | Where-Object { $_ -le 0 }).Count -eq 0)) 'Scheduler-loop frame callback counters failed.' $gdbStdout
    $mv = Get-Ints $move
    Assert-Condition ($mv[6] -ne 0 -and $mv[7] -ne 0 -and $mv[8] -gt 0 -and $mv[9] -gt 0 -and $mv[10] -eq 1 -and $mv[11] -eq 1 -and $mv[12] -eq 1 -and $mv[13] -eq 1) 'Scheduler-loop movement/floor markers failed.' $gdbStdout
    $tr = Get-Ints $trans
    Assert-Condition ($tr[0] -ge 2 -and $tr[1] -ge 2 -and $tr[2] -ge 2 -and $tr[3] -ge 2 -and $tr[4] -ge 4 -and $tr[5] -ge 2 -and $tr[6] -ge 2 -and $tr[7] -ge 2 -and $tr[8] -gt 0) 'Scheduler-loop transition counters failed.' $gdbStdout
    $sf = Get-Ints $safe
    Assert-Condition ((@($sf[0..9] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'Scheduler-loop safe escape counters were not zero.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after scheduler-loop proof.' $gdbStdout
    Write-Output ("$Label harness passed: scene=22/21 updates={0} callbacks={1}/{2} visits=0x{3:x}/0x{4:x} transitions=0x{5:x}/0x{6:x} root-dx={7}/{8} rise={9}/{10} final=Wait/Ground safe=1" -f $tm[1], $pr[4], $pr[5], $st[10], $st[11], $st[12], $st[13], $mv[6], $mv[7], $mv[8], $mv[9])
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
