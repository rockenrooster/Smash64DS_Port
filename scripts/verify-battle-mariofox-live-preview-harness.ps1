param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [string]$Harness = 'battle_mariofox_live_preview',
    [string]$Target = 'smash64ds-battle-mariofox-live-preview',
    [string]$Build = 'build-battle-mariofox-live-preview-harness',
    [int]$ExpectedMode = 55,
    [int]$ExpectedHarnessSceneCurr = 22,
    [int]$ExpectedHarnessScenePrev = 21,
    [string]$Label = 'Battle Mario/Fox live-preview',
    [string]$HarnessSelectMessage = 'Direct live-preview harness did not select VSBattle from Maps.'
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
        'printf "VS_TRANS=%#x,%#x\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask',
        'printf "PV_TRANS=%#x,%#x\n", gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask',
        'printf "MAPS_TRANS=%#x,%#x,%u\n", gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask, gNdsMapsSelectTransitionSelectedGKind',
        'printf "GCRUNALL_LOOP=%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterMarioFoxGCRunAllLoopResult, gNdsFighterMarioFoxGCRunAllLoopSafeResult, gNdsFighterMarioFoxGCRunAllLoopMask, gNdsFighterMarioFoxGCRunAllLoopDeferredMask, gNdsFighterMarioFoxGCRunAllLoopCount, gNdsFighterGCRunAllLoopP0StatusFinal, gNdsFighterGCRunAllLoopP1StatusFinal, gNdsFighterGCRunAllLoopP0MotionFinal, gNdsFighterGCRunAllLoopP1MotionFinal, gNdsFighterGCRunAllLoopP0GAFinal, gNdsFighterGCRunAllLoopP1GAFinal, gNdsFighterGCRunAllLoopRunAllCount',
        'printf "LIVE_LOOP=%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsFighterMarioFoxLivePreviewResult, gNdsFighterMarioFoxLivePreviewSafeResult, gNdsFighterMarioFoxLivePreviewMask, gNdsFighterMarioFoxLivePreviewDeferredMask, gNdsFighterMarioFoxLivePreviewCount, gNdsFighterLivePreviewDevMode, gNdsFighterLivePreviewIdleFrameTarget, gNdsFighterLivePreviewFrameMax, gNdsFighterLivePreviewUpdateMax',
        'printf "LIVE_BACKEND=%u,%u,%u,%u,%u,%u,%#x,%u,%u,%d,%d\n", gNdsControllerPlaybackEnabled, gNdsFighterLivePreviewLiveReadDelta, gNdsFighterLivePreviewPlaybackReadDelta, gNdsFighterLivePreviewSYReadCount, gNdsFighterLivePreviewSYUpdateCount, gNdsControllerLiveMapCount, gNdsFighterLivePreviewControllerLiveConnectedMask, gNdsFighterLivePreviewAnyInputSeen, gNdsControllerLivePad0Button, gNdsControllerLivePad0StickX, gNdsControllerLivePad0StickY',
        'printf "LIVE_TASKMAN=%u,%u,%u,%u,%u,%u\n", gNdsFighterLivePreviewPrepared, gNdsFighterLivePreviewTaskmanUpdateCount, gNdsFighterLivePreviewVSBattleUpdateCount, gNdsFighterLivePreviewBaseVSBattleUpdateCount, gNdsFighterLivePreviewRunAllCount, gNdsTaskmanBoundedUpdateCount',
        'printf "LIVE_RUN=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterLivePreviewP0ProcessAttachCount, gNdsFighterLivePreviewP1ProcessAttachCount, gNdsFighterLivePreviewP0GObjProcessRunCount, gNdsFighterLivePreviewP1GObjProcessRunCount, gNdsFighterLivePreviewP0ProcCallbackCount, gNdsFighterLivePreviewP1ProcCallbackCount, gNdsFighterLivePreviewManualGObjProcessRunCount, gNdsFighterLivePreviewNonTargetProcCallbackCount, gNdsFighterLivePreviewProcessAttachEscapeCount',
        'printf "LIVE_INPUT=%u,%u,%u,%u,%#x,%#x,%#x,%#x,%d,%d,%d,%d,%u,%u\n", gNdsFighterLivePreviewP0ControllerToFTInputCount, gNdsFighterLivePreviewP1ControllerToFTInputCount, gNdsFighterLivePreviewP0DirectFTInputWriteCount, gNdsFighterLivePreviewP1DirectFTInputWriteCount, gNdsFighterLivePreviewP0ButtonTapMask, gNdsFighterLivePreviewP1ButtonTapMask, gNdsFighterLivePreviewP0ButtonHoldMask, gNdsFighterLivePreviewP1ButtonHoldMask, gNdsFighterLivePreviewP0LastStickX, gNdsFighterLivePreviewP1LastStickX, gNdsFighterLivePreviewP0LastStickY, gNdsFighterLivePreviewP1LastStickY, gNdsFighterLivePreviewP0DashTapEligibleCount, gNdsFighterLivePreviewP1DashTapEligibleCount',
        'printf "LIVE_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%u,%u,%u,%u\n", gNdsFighterLivePreviewP0StatusStart, gNdsFighterLivePreviewP1StatusStart, gNdsFighterLivePreviewP0MotionStart, gNdsFighterLivePreviewP1MotionStart, gNdsFighterLivePreviewP0StatusFinal, gNdsFighterLivePreviewP1StatusFinal, gNdsFighterLivePreviewP0MotionFinal, gNdsFighterLivePreviewP1MotionFinal, gNdsFighterLivePreviewP0GAFinal, gNdsFighterLivePreviewP1GAFinal, gNdsFighterLivePreviewP0StatusVisitMask, gNdsFighterLivePreviewP1StatusVisitMask, gNdsFighterLivePreviewP0TransitionMask, gNdsFighterLivePreviewP1TransitionMask, gNdsFighterLivePreviewP0Completed, gNdsFighterLivePreviewP1Completed, gNdsFighterLivePreviewP0FrameCount, gNdsFighterLivePreviewP1FrameCount',
        'printf "LIVE_CALLS=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterLivePreviewP0InterruptCount, gNdsFighterLivePreviewP1InterruptCount, gNdsFighterLivePreviewP0PhysicsCount, gNdsFighterLivePreviewP1PhysicsCount, gNdsFighterLivePreviewP0IntegrateCount, gNdsFighterLivePreviewP1IntegrateCount, gNdsFighterLivePreviewP0MapCount, gNdsFighterLivePreviewP1MapCount',
        'printf "LIVE_DRAW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterLivePreviewPreviewWidth, gNdsFighterLivePreviewPreviewHeight, gNdsFighterLivePreviewPreviewPitch, gNdsFighterLivePreviewPreviewReady, gNdsFighterLivePreviewPreviewCommitDelta, gNdsFighterLivePreviewDrawFrameCount, gNdsFighterLivePreviewDisplayCallbackCount, gNdsFighterLivePreviewP0DisplayCallbackCount, gNdsFighterLivePreviewP1DisplayCallbackCount, gNdsFighterLivePreviewP0CandidateCount, gNdsFighterLivePreviewP1CandidateCount, gNdsFighterLivePreviewP0DrawnDObjCount, gNdsFighterLivePreviewP1DrawnDObjCount, gNdsFighterLivePreviewTotalPixelCount, gNdsFighterLivePreviewP0ColorChecksum, gNdsFighterLivePreviewP1ColorChecksum',
        'printf "LIVE_MOVE=%d,%d,%d,%d,%d,%d,%u,%u\n", gNdsFighterLivePreviewP0FloorYMilli, gNdsFighterLivePreviewP1FloorYMilli, gNdsFighterLivePreviewP0RootDeltaXMilli, gNdsFighterLivePreviewP1RootDeltaXMilli, gNdsFighterLivePreviewP0RootYFinalMilli, gNdsFighterLivePreviewP1RootYFinalMilli, gNdsFighterLivePreviewP0FloorOK, gNdsFighterLivePreviewP1FloorOK',
        'printf "LIVE_SAFE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterLivePreviewGObjDelta, gNdsFighterLivePreviewUnexpectedStatusCount, gNdsFighterLivePreviewDeniedStatusCount, gNdsFighterLivePreviewProcessAttachEscapeCount, gNdsFighterLivePreviewDisplayProbeCount, gNdsFighterLivePreviewGameplayUpdateCount, gNdsFighterLivePreviewDrawCallCount, gNdsFighterLivePreviewMatrixCallCount, gNdsFighterLivePreviewRootYDriftCount, gNdsFighterLivePreviewGADriftCount',
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
    $gcrun = [regex]::Match($gdbStdout, 'GCRUNALL_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $loop = [regex]::Match($gdbStdout, 'LIVE_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $backend = [regex]::Match($gdbStdout, 'LIVE_BACKEND=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $taskman = [regex]::Match($gdbStdout, 'LIVE_TASKMAN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $run = [regex]::Match($gdbStdout, 'LIVE_RUN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'LIVE_INPUT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $status = [regex]::Match($gdbStdout, 'LIVE_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $calls = [regex]::Match($gdbStdout, 'LIVE_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $draw = [regex]::Match($gdbStdout, 'LIVE_DRAW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $move = [regex]::Match($gdbStdout, 'LIVE_MOVE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'LIVE_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platform = [regex]::Match($gdbStdout, 'PLATFORM_DL_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq $ExpectedMode -and [int]$harn.Groups[3].Value -eq $ExpectedHarnessSceneCurr -and [int]$harn.Groups[4].Value -eq $ExpectedHarnessScenePrev -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) $HarnessSelectMessage $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene is not Pupupu VSBattle from Maps.' $gdbStdout
    if ($ExpectedMode -eq 56) {
        Assert-Condition ($vs.Success -and (Convert-MarkerUInt32 $vs.Groups[1].Value) -eq 0x56535452 -and ((Convert-MarkerUInt32 $vs.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain VS Mode -> PlayersVS transition did not pass.' $gdbStdout
        Assert-Condition ($pv.Success -and (Convert-MarkerUInt32 $pv.Groups[1].Value) -eq 0x50565452 -and ((Convert-MarkerUInt32 $pv.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain PlayersVS -> Maps transition did not pass.' $gdbStdout
        Assert-Condition ($maps.Success -and (Convert-MarkerUInt32 $maps.Groups[1].Value) -eq 0x4d53454c -and ((Convert-MarkerUInt32 $maps.Groups[2].Value) -band 0xff) -eq 0xff -and [int]$maps.Groups[3].Value -eq 6) 'Menu-chain Maps -> VSBattle transition did not pass.' $gdbStdout
    }
    $gc = Get-Ints $gcrun
    Assert-Condition ($gcrun.Success -and $gc[0] -eq 0x4647414c -and $gc[1] -eq 0x46474153 -and (($gc[2] -band 0x7ff) -eq 0x7ff) -and $gc[3] -eq 0xff -and $gc[4] -eq 2 -and ($gc[5..10] -join ',') -eq '10,10,4,4,0,0' -and $gc[11] -gt 0) 'Prerequisite gcRunAll-loop proof did not pass.' $gdbStdout
    $lp = Get-Ints $loop
    Assert-Condition ($loop.Success -and $lp[0] -eq 0x464c5650 -and $lp[1] -eq 0x464c5653 -and (($lp[2] -band 0x7ff) -eq 0x7ff) -and $lp[3] -eq 0xff -and $lp[4] -eq 2 -and $lp[5] -eq 0 -and $lp[6] -eq 60 -and $lp[7] -ge 60 -and $lp[8] -ge 60) 'Live-preview result/mask did not pass.' $gdbStdout
    $be = Get-Ints $backend
    Assert-Condition ($be[0] -eq 0 -and $be[1] -gt 0 -and $be[2] -eq 0 -and $be[3] -eq $be[1] -and $be[4] -eq $be[1] -and $be[5] -ge $be[1] -and (($be[6] -band 1) -eq 1) -and $be[7] -eq 0 -and $be[8] -eq 0 -and $be[9] -eq 0 -and $be[10] -eq 0) 'Live controller backend path was not neutral live input.' $gdbStdout
    $tm = Get-Ints $taskman
    Assert-Condition ($tm[0] -eq 1 -and $tm[1] -ge 60 -and $tm[2] -ge 60 -and $tm[3] -eq $tm[2] -and $tm[4] -ge 60 -and $tm[5] -ge $tm[1]) 'Live taskman/gcRunAll update path failed.' $gdbStdout
    $rn = Get-Ints $run
    Assert-Condition ($rn[0] -eq 1 -and $rn[1] -eq 1 -and $rn[2] -ge 60 -and $rn[3] -ge 60 -and $rn[4] -eq $rn[2] -and $rn[5] -eq $rn[3] -and $rn[6] -eq 0 -and $rn[7] -eq 0 -and $rn[8] -eq 0) 'Live process attach/callback path failed.' $gdbStdout
    $inp = Get-Ints $input
    Assert-Condition ($inp[0] -ge 60 -and $inp[1] -ge 60 -and $inp[2] -eq 0 -and $inp[3] -eq 0 -and $inp[4] -eq 0 -and $inp[5] -eq 0 -and $inp[6] -eq 0 -and $inp[7] -eq 0 -and $inp[8] -eq 0 -and $inp[9] -eq 0 -and $inp[10] -eq 0 -and $inp[11] -eq 0 -and $inp[12] -eq 0 -and $inp[13] -eq 0) 'Live input bridge was not neutral or wrote FTStruct directly.' $gdbStdout
    $st = Get-Ints $status
    Assert-Condition (($st[0..9] -join ',') -eq '10,10,4,4,10,10,4,4,0,0' -and (($st[10] -band 1) -eq 1) -and (($st[11] -band 1) -eq 1) -and $st[12] -eq 0 -and $st[13] -eq 0 -and $st[14] -eq 1 -and $st[15] -eq 1 -and $st[16] -ge 60 -and $st[17] -ge 60) 'Live idle status/final state failed.' $gdbStdout
    $ca = Get-Ints $calls
    Assert-Condition ((@($ca | Where-Object { $_ -lt 60 }).Count -eq 0)) 'Live callback frame counters failed.' $gdbStdout
    $dr = Get-Ints $draw
    Assert-Condition ($dr[0] -eq 96 -and $dr[1] -eq 72 -and $dr[2] -ge 96 -and $dr[3] -eq 1 -and $dr[4] -ge 5 -and $dr[5] -ge 5 -and $dr[6] -ge 10 -and $dr[7] -ge 5 -and $dr[8] -ge 5 -and $dr[9] -ge 14 -and $dr[10] -ge 18 -and $dr[11] -ge 14 -and $dr[12] -ge 18 -and $dr[13] -gt 0 -and $dr[14] -ne 0 -and $dr[15] -ne 0) 'Live draw/DObj/pixel markers failed.' $gdbStdout
    $mv = Get-Ints $move
    Assert-Condition ($mv[2] -eq 0 -and $mv[3] -eq 0 -and $mv[4] -eq $mv[0] -and $mv[5] -eq $mv[1] -and $mv[6] -eq 1 -and $mv[7] -eq 1) 'Live idle movement/floor markers failed.' $gdbStdout
    $sf = Get-Ints $safe
    Assert-Condition ((@($sf[0..9] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'Live safe escape counters were not zero.' $gdbStdout
    $pd = Get-Ints $platform
    Assert-Condition ($pd[0] -eq 1 -and $pd[1] -eq 96 -and $pd[2] -eq 72 -and $pd[3] -ge $dr[4]) 'Platform original-DL preview markers failed.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after live proof.' $gdbStdout
    Write-Output ("$Label harness passed: scene=22/21 liveReads={0} frames={1}/{2} draws={3} pixels={4} idle=Wait/Ground directInput=0 safe=1" -f $be[1], $st[16], $st[17], $dr[5], $dr[13])
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
