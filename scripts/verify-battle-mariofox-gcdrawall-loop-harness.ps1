param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [string]$Harness = 'battle_mariofox_gcdrawall_loop',
    [string]$Target = 'smash64ds-battle-mariofox-gcdrawall-loop',
    [string]$Build = 'build-battle-mariofox-gcdrawall-loop-harness',
    [int]$ExpectedMode = 57,
    [int]$ExpectedHarnessSceneCurr = 22,
    [int]$ExpectedHarnessScenePrev = 21,
    [string]$Label = 'Battle Mario/Fox gcDrawAll-loop',
    [string]$HarnessSelectMessage = 'Direct gcDrawAll-loop harness did not select VSBattle from Maps.',
    [switch]$HardwareTriangles,
    [switch]$RequireStageDraw,
    [switch]$RequireStageCollision,
    [switch]$RequireStageFloorFollow,
    [switch]$RequireStageFloorEdge,
    [switch]$RequireStageMPProcessFloor,
    [switch]$RequireStageMPUpdateFloor,
    [switch]$RequireStageMPSweepFloor,
    [switch]$RequireStageMPCrossFloor,
    [switch]$RequireStageMPAdjustFloor,
    [switch]$RequireStageMPEdgeFloor,
    [switch]$RequireStageMPWallFloor,
    [switch]$RequireStageMPWallHitFloor,
    [switch]$RequireStageMPWallCopyFloor,
    [switch]$RequireStageMPPassFloor,
    [switch]$RequireStageMPPlatformFloor,
    [switch]$RequireStageMPPlatformActiveFloor,
    [switch]$RequireStageMPPlatformTickFloor,
    [switch]$RequireStageMPPassInputLoop,
    [switch]$RequireStageMPPlatformPosFloor,
    [switch]$RequireStageMPPlatformSpeedFloor,
    [switch]$RequireStageInishieScaleLoop,
    [switch]$RequireStageMPStaleFloor,
    [switch]$RequireStageMPLiveStaleFloor,
    [switch]$RequireStageMPMotionStaleFloor,
    [switch]$RequireStageMPCliffStatusFloor,
    [switch]$RequireStageMPCliffTickFloor,
    [switch]$RequireStageMPFallMapFloor,
    [switch]$RequireStageMPFallLandFloor,
    [switch]$RequireStageMPCeilFloor,
    [switch]$RequireStageMPCeilStatusFloor,
    [switch]$RequireStageMPCliffCatchFloor,
    [switch]$RequireStageMPCliffWaitFloor,
    [switch]$RequireStageMPCliffAttackFloor,
    [switch]$RequireStageMPCliffAttackAction,
    [switch]$RequireStageMPCliffCommon2,
    [switch]$RequireStageMPCliffEscapeAction,
    [switch]$RequireStageMPCliffEscapeCommon2,
    [switch]$RequireStageMPCliffClimbFloor,
    [switch]$RequireStageMPCliffClimbAction,
    [switch]$RequireStageMPCliffClimbCommon2,
    [switch]$RequireStageMPCliffClimbFinish,
    [switch]$RequireStageMPCliffWaitDamage,
    [switch]$RequireStageMPPassiveLoop,
    [switch]$RequireStageMPPassiveRecoverLoop,
    [switch]$RequireStageMPDamageRecoverLoop,
    [switch]$RequireStageMPLiveHitDamageLoop,
    [switch]$RequireStageMPLiveHitStatusLoop,
    [switch]$RequireStageMPDownWaitLoop,
    [switch]$RequireStageTurnLoop,
    [switch]$RequireStageMPDownRecoverLoop,
    [switch]$RequireStageMPCliffLedgeLoop,
    [switch]$RequireStageMPCliffLiveLoop
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
if ($RequireStageMPCrossFloor) {
    $RequireStageMPSweepFloor = $true
}
if ($RequireStageMPAdjustFloor) {
    $RequireStageMPCrossFloor = $true
    $RequireStageMPSweepFloor = $true
}
if ($RequireStageMPEdgeFloor) {
    $RequireStageMPAdjustFloor = $true
    $RequireStageMPCrossFloor = $true
    $RequireStageMPSweepFloor = $true
}
if ($RequireStageMPWallFloor) {
    $RequireStageMPEdgeFloor = $true
    $RequireStageMPAdjustFloor = $true
    $RequireStageMPCrossFloor = $true
    $RequireStageMPSweepFloor = $true
}
if ($RequireStageMPWallHitFloor -and -not $RequireStageMPCliffLiveLoop) {
    $RequireStageMPWallFloor = $true
    $RequireStageMPEdgeFloor = $true
    $RequireStageMPAdjustFloor = $true
    $RequireStageMPCrossFloor = $true
    $RequireStageMPSweepFloor = $true
}
if ($RequireStageMPWallCopyFloor) {
    $RequireStageMPWallHitFloor = $true
    $RequireStageMPCliffLiveLoop = $true
}
if ($RequireStageMPPassFloor) {
    $RequireStageMPWallCopyFloor = $true
    $RequireStageMPWallHitFloor = $true
    $RequireStageMPCliffLiveLoop = $true
}
if ($RequireStageMPPlatformActiveFloor) {
    $RequireStageMPPlatformFloor = $true
}
if ($RequireStageMPPlatformTickFloor) {
    $RequireStageMPPlatformActiveFloor = $true
    $RequireStageMPPlatformFloor = $true
}
if ($RequireStageMPPassInputLoop) {
    $RequireStageMPPlatformTickFloor = $true
    $RequireStageMPPlatformActiveFloor = $true
    $RequireStageMPPlatformFloor = $true
}
if ($RequireStageMPPlatformPosFloor) {
    $RequireStageMPPassInputLoop = $true
    $RequireStageMPPlatformTickFloor = $true
    $RequireStageMPPlatformActiveFloor = $true
    $RequireStageMPPlatformFloor = $true
}
if ($RequireStageMPPlatformSpeedFloor) {
    $RequireStageMPPlatformPosFloor = $true
    $RequireStageMPPassInputLoop = $true
    $RequireStageMPPlatformTickFloor = $true
    $RequireStageMPPlatformActiveFloor = $true
    $RequireStageMPPlatformFloor = $true
}
if ($RequireStageInishieScaleLoop) {
    $RequireStageMPPlatformSpeedFloor = $true
    $RequireStageMPPlatformPosFloor = $true
    $RequireStageMPPassInputLoop = $true
    $RequireStageMPPlatformTickFloor = $true
    $RequireStageMPPlatformActiveFloor = $true
    $RequireStageMPPlatformFloor = $true
}
if ($RequireStageMPPlatformFloor) {
    $RequireStageMPPassFloor = $true
}
if ($RequireStageMPLiveStaleFloor) {
    $RequireStageMPStaleFloor = $true
}
if ($RequireStageMPCliffStatusFloor) {
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPCliffTickFloor) {
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPFallMapFloor) {
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPFallLandFloor) {
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPCeilFloor) {
    $RequireStageMPFallLandFloor = $true
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPCeilStatusFloor) {
    $RequireStageMPCeilFloor = $true
    $RequireStageMPFallLandFloor = $true
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPCliffCatchFloor) {
    $RequireStageMPCeilStatusFloor = $true
    $RequireStageMPCeilFloor = $true
    $RequireStageMPFallLandFloor = $true
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPCliffWaitFloor) {
    $RequireStageMPCliffCatchFloor = $true
    $RequireStageMPCeilStatusFloor = $true
    $RequireStageMPCeilFloor = $true
    $RequireStageMPFallLandFloor = $true
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPCliffAttackFloor) {
    $RequireStageMPCliffWaitFloor = $true
    $RequireStageMPCliffCatchFloor = $true
    $RequireStageMPCeilStatusFloor = $true
    $RequireStageMPCeilFloor = $true
    $RequireStageMPFallLandFloor = $true
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPDamageRecoverLoop) {
    $RequireStageMPPassiveRecoverLoop = $true
}
if ($RequireStageMPLiveHitDamageLoop) {
    $RequireStageMPDamageRecoverLoop = $true
    $RequireStageMPPassiveRecoverLoop = $true
}
if ($RequireStageMPLiveHitStatusLoop) {
    $RequireStageMPLiveHitDamageLoop = $true
    $RequireStageMPDamageRecoverLoop = $true
    $RequireStageMPPassiveRecoverLoop = $true
}
if ($RequireStageMPPassiveRecoverLoop) {
    $RequireStageMPPassiveLoop = $true
    $RequireStageMPDownWaitLoop = $true
    $RequireStageTurnLoop = $true
    $RequireStageMPDownRecoverLoop = $true
    $RequireStageMPCliffLedgeLoop = $true
    $RequireStageMPCliffLiveLoop = $true
    $RequireStageMPCliffCommon2 = $true
    $RequireStageMPCliffEscapeCommon2 = $true
    $RequireStageMPWallHitFloor = $true
    $RequireStageMPWallCopyFloor = $true
    $RequireStageMPPassFloor = $true
    $RequireStageInishieScaleLoop = $true
    $RequireStageMPPlatformSpeedFloor = $true
    $RequireStageMPPlatformPosFloor = $true
    $RequireStageMPPassInputLoop = $true
    $RequireStageMPPlatformTickFloor = $true
    $RequireStageMPPlatformActiveFloor = $true
    $RequireStageMPPlatformFloor = $true
}
if ($RequireStageMPDownRecoverLoop) {
    $RequireStageTurnLoop = $true
}
if ($RequireStageMPCliffLiveLoop) {
    $RequireStageMPCliffLedgeLoop = $true
}
if ($RequireStageMPCliffLedgeLoop) {
    $RequireStageMPDownRecoverLoop = $true
    $RequireStageTurnLoop = $true
    $RequireStageMPDownWaitLoop = $true
    $RequireStageMPPassiveLoop = $true
    $RequireStageMPCliffWaitDamage = $true
    $RequireStageMPCliffClimbFinish = $true
    $RequireStageMPCliffClimbCommon2 = $true
    $RequireStageMPCliffClimbAction = $true
    $RequireStageMPCliffClimbFloor = $true
    $RequireStageMPCliffWaitFloor = $true
    $RequireStageMPCliffCatchFloor = $true
}
if ($RequireStageTurnLoop) {
    $RequireStageMPDownWaitLoop = $true
}
if ($RequireStageMPDownWaitLoop) {
    $RequireStageMPPassiveLoop = $true
    $RequireStageMPCliffWaitDamage = $true
}
if ($RequireStageMPCliffWaitDamage) {
    $RequireStageMPCliffWaitFloor = $true
    $RequireStageMPCliffCatchFloor = $true
    $RequireStageMPCeilStatusFloor = $true
    $RequireStageMPCeilFloor = $true
    $RequireStageMPFallLandFloor = $true
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPCliffClimbFinish) {
    $RequireStageMPCliffClimbCommon2 = $true
}
if ($RequireStageMPCliffClimbCommon2) {
    $RequireStageMPCliffClimbAction = $true
}
if ($RequireStageMPCliffClimbAction) {
    $RequireStageMPCliffClimbFloor = $true
}
if ($RequireStageMPCliffClimbFloor) {
    $RequireStageMPCliffWaitFloor = $true
    $RequireStageMPCliffCatchFloor = $true
    $RequireStageMPCeilStatusFloor = $true
    $RequireStageMPCeilFloor = $true
    $RequireStageMPFallLandFloor = $true
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPCliffAttackAction) {
    $RequireStageMPCliffAttackFloor = $true
    $RequireStageMPCliffWaitFloor = $true
    $RequireStageMPCliffCatchFloor = $true
    $RequireStageMPCeilStatusFloor = $true
    $RequireStageMPCeilFloor = $true
    $RequireStageMPFallLandFloor = $true
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPCliffCommon2) {
    $RequireStageMPCliffAttackAction = $true
    $RequireStageMPCliffAttackFloor = $true
    $RequireStageMPCliffWaitFloor = $true
    $RequireStageMPCliffCatchFloor = $true
    $RequireStageMPCeilStatusFloor = $true
    $RequireStageMPCeilFloor = $true
    $RequireStageMPFallLandFloor = $true
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPCliffEscapeCommon2) {
    $RequireStageMPCliffEscapeAction = $true
}
if ($RequireStageMPCliffEscapeAction) {
    $RequireStageMPCliffWaitFloor = $true
    $RequireStageMPCliffCatchFloor = $true
    $RequireStageMPCeilStatusFloor = $true
    $RequireStageMPCeilFloor = $true
    $RequireStageMPFallLandFloor = $true
    $RequireStageMPFallMapFloor = $true
    $RequireStageMPCliffTickFloor = $true
    $RequireStageMPCliffStatusFloor = $true
    $RequireStageMPMotionStaleFloor = $true
}
if ($RequireStageMPStaleFloor) {
    $RequireStageMPWallFloor = $true
    $RequireStageMPEdgeFloor = $true
    $RequireStageMPAdjustFloor = $true
    $RequireStageMPCrossFloor = $true
    $RequireStageMPSweepFloor = $true
}
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$selectedGdbPort = if (($RunnerSlot -ge 0) -and -not $PSBoundParameters.ContainsKey('GdbPort')) {
    Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
} else {
    $GdbPort
}
Set-MelonDSVerifierRunContext -Root $root -RunnerSlot $RunnerSlot -GdbPort $selectedGdbPort
if ($NoBuild) { $env:SMASH64DS_VERIFY_NO_BUILD = '1' }
if ($RunnerSlot -ge 0) {
    Resolve-MelonDSRunnerSlot `
        -Root $root `
        -RunnerSlot $RunnerSlot `
        -MelonDS $MelonDS `
        -GdbPort $selectedGdbPort `
        -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') | Out-Null
}
$rom = Join-Path $root "$Target.nds"
$elf = Join-Path $root "$Target.elf"
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
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
function Assert-CliffCommon2BridgeRoot {
    param(
        [System.Text.RegularExpressions.Match]$Match,
        [int]$ExpectedStatusID,
        [string]$Message,
        [string]$Context
    )
    $v = Get-Ints $Match
    $rootChanged = $false
    $rootExpected = $false
    if ($Match.Success) {
        $rootChanged = ($v[6] -ne $v[8]) -or ($v[7] -ne $v[9])
        $rootExpected = ([Math]::Abs($v[8] - $v[10]) -le 1) -and
            ([Math]::Abs($v[9] - $v[11]) -le 1)
    }
    Assert-Condition ($Match.Success -and $v[0] -eq 1 -and $v[1] -eq 1 -and
        $v[2] -eq 0 -and $v[3] -eq $ExpectedStatusID -and $v[5] -ge 0 -and
        $rootChanged -and $rootExpected -and ([Math]::Abs($v[12]) -le 1) -and
        $v[13] -eq 1) $Message $Context
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$makeArgs = @('-C', $root, "TARGET=$Target", "BUILD=$Build", "NDS_DEV_SCENE_HARNESS=$Harness", '-j16')
if ($HardwareTriangles) { $makeArgs += 'NDS_RENDERER_HW_TRIANGLES=1' }
& make @makeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw "$Label harness build did not produce the expected ROM and ELF."
}
if (-not (Test-Path $melonDsPath)) { throw "melonDS executable not found: $melonDsPath" }
if (-not (Test-Path $Gdb)) { throw "GDB executable not found: $Gdb" }
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $melonDsPath -GdbPort $selectedGdbPort -Persistent:($RunnerSlot -ge 0)
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $selectedGdbPort | Out-Null
    Start-Sleep -Seconds ([Math]::Max($DelaySeconds, 1))
    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 5',`
        ("target remote 127.0.0.1:{0}" -f (Get-MelonDSActiveGdbPort)),
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind',
        'printf "VS_TRANS=%#x,%#x\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask',
        'printf "PV_TRANS=%#x,%#x\n", gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask',
        'printf "MAPS_TRANS=%#x,%#x,%u\n", gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask, gNdsMapsSelectTransitionSelectedGKind',
        'printf "SCHED_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxSchedulerLoopResult, gNdsFighterMarioFoxSchedulerLoopSafeResult, gNdsFighterMarioFoxSchedulerLoopMask, gNdsFighterMarioFoxSchedulerLoopDeferredMask, gNdsFighterMarioFoxSchedulerLoopCount',
        'printf "CTRL_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxControllerLoopResult, gNdsFighterMarioFoxControllerLoopSafeResult, gNdsFighterMarioFoxControllerLoopMask, gNdsFighterMarioFoxControllerLoopDeferredMask, gNdsFighterMarioFoxControllerLoopCount',
        'printf "PREVIEW_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxPreviewLoopResult, gNdsFighterMarioFoxPreviewLoopSafeResult, gNdsFighterMarioFoxPreviewLoopMask, gNdsFighterMarioFoxPreviewLoopDeferredMask, gNdsFighterMarioFoxPreviewLoopCount',
        'printf "GCRUNALL_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxGCRunAllLoopResult, gNdsFighterMarioFoxGCRunAllLoopSafeResult, gNdsFighterMarioFoxGCRunAllLoopMask, gNdsFighterMarioFoxGCRunAllLoopDeferredMask, gNdsFighterMarioFoxGCRunAllLoopCount',
        'printf "GCDRAWALL_LOOP=%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxGCDrawAllLoopResult, gNdsFighterMarioFoxGCDrawAllLoopSafeResult, gNdsFighterMarioFoxGCDrawAllLoopMask, gNdsFighterMarioFoxGCDrawAllLoopDeferredMask, gNdsFighterMarioFoxGCDrawAllLoopCount, gNdsFighterGCDrawAllLoopFrameMax, gNdsFighterGCDrawAllLoopUpdateMax',
        'printf "GCDRAWALL_TASKMAN=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCDrawAllLoopPrepared, gNdsFighterGCDrawAllLoopTaskmanUpdateCount, gNdsFighterGCDrawAllLoopVSBattleUpdateCount, gNdsFighterGCDrawAllLoopBaseVSBattleUpdateCount, gNdsFighterGCDrawAllLoopRunAllCount, gNdsFighterGCDrawAllLoopDrawAllCount, gNdsFighterGCDrawAllLoopCameraCallbackCount, gNdsTaskmanBoundedUpdateCount',
        'printf "GCDRAWALL_RUN=%u,%u,%u,%u,%u,%u\n", gNdsFighterGCDrawAllLoopOldProcessPauseCount, gNdsFighterGCDrawAllLoopNonTargetGObjVisitCount, gNdsFighterGCDrawAllLoopNonTargetProcessPauseCount, gNdsFighterGCDrawAllLoopTargetProcessPreserveCount, gNdsFighterGCDrawAllLoopGObjCountBefore, gNdsFighterGCDrawAllLoopGObjCountAfter',
        'printf "GCDRAWALL_PROCESS=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCDrawAllLoopP0ProcessAttachCount, gNdsFighterGCDrawAllLoopP1ProcessAttachCount, gNdsFighterGCDrawAllLoopP0GObjProcessRunCount, gNdsFighterGCDrawAllLoopP1GObjProcessRunCount, gNdsFighterGCDrawAllLoopP0ProcCallbackCount, gNdsFighterGCDrawAllLoopP1ProcCallbackCount, gNdsFighterGCDrawAllLoopProcessAttachEscapeCount',
        'printf "GCDRAWALL_INPUT=%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterGCDrawAllLoopP0PlaybackApplyCount, gNdsFighterGCDrawAllLoopP1PlaybackApplyCount, gNdsFighterGCDrawAllLoopP0ControllerToFTInputCount, gNdsFighterGCDrawAllLoopP1ControllerToFTInputCount, gNdsFighterGCDrawAllLoopP0DirectFTInputWriteCount, gNdsFighterGCDrawAllLoopP1DirectFTInputWriteCount, gNdsFighterGCDrawAllLoopP0DashTapEligibleCount, gNdsFighterGCDrawAllLoopP1DashTapEligibleCount, gNdsFighterGCDrawAllLoopP0ButtonTapMask, gNdsFighterGCDrawAllLoopP1ButtonTapMask, gNdsFighterGCDrawAllLoopP0ButtonHoldMask, gNdsFighterGCDrawAllLoopP1ButtonHoldMask, gNdsFighterGCDrawAllLoopP0JumpButtonTapCount, gNdsFighterGCDrawAllLoopP1JumpButtonTapCount',
        'printf "GCDRAWALL_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsFighterGCDrawAllLoopP0StatusStart, gNdsFighterGCDrawAllLoopP1StatusStart, gNdsFighterGCDrawAllLoopP0MotionStart, gNdsFighterGCDrawAllLoopP1MotionStart, gNdsFighterGCDrawAllLoopP0StatusFinal, gNdsFighterGCDrawAllLoopP1StatusFinal, gNdsFighterGCDrawAllLoopP0MotionFinal, gNdsFighterGCDrawAllLoopP1MotionFinal, gNdsFighterGCDrawAllLoopP0GAFinal, gNdsFighterGCDrawAllLoopP1GAFinal, gNdsFighterGCDrawAllLoopP0StatusVisitMask, gNdsFighterGCDrawAllLoopP1StatusVisitMask, gNdsFighterGCDrawAllLoopP0TransitionMask, gNdsFighterGCDrawAllLoopP1TransitionMask',
        'printf "GCDRAWALL_DRAW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%u\n", gNdsFighterGCDrawAllLoopPreviewWidth, gNdsFighterGCDrawAllLoopPreviewHeight, gNdsFighterGCDrawAllLoopPreviewPitch, gNdsFighterGCDrawAllLoopPreviewReady, gNdsFighterGCDrawAllLoopPreviewCommitDelta, gNdsFighterGCDrawAllLoopDrawFrameCount, gNdsFighterGCDrawAllLoopDisplayCallbackCount, gNdsFighterGCDrawAllLoopP0DisplayCallbackCount, gNdsFighterGCDrawAllLoopP1DisplayCallbackCount, gNdsFighterGCDrawAllLoopP0CandidateCount, gNdsFighterGCDrawAllLoopP1CandidateCount, gNdsFighterGCDrawAllLoopP0DrawnDObjCount, gNdsFighterGCDrawAllLoopP1DrawnDObjCount, gNdsFighterGCDrawAllLoopTotalPixelCount, gNdsFighterGCDrawAllLoopP0ColorChecksum, gNdsFighterGCDrawAllLoopP1ColorChecksum, gNdsFighterGCDrawAllLoopCapturedDisplayCount, gNdsFighterGCDrawAllLoopNonTargetDisplayCallbackCount',
        'printf "GCDRAWALL_SCREEN=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsFighterGCDrawAllLoopP0ScreenXStart, gNdsFighterGCDrawAllLoopP1ScreenXStart, gNdsFighterGCDrawAllLoopP0ScreenXFinal, gNdsFighterGCDrawAllLoopP1ScreenXFinal, gNdsFighterGCDrawAllLoopP0ScreenXDelta, gNdsFighterGCDrawAllLoopP1ScreenXDelta, gNdsFighterGCDrawAllLoopP0ScreenYFloor, gNdsFighterGCDrawAllLoopP1ScreenYFloor, gNdsFighterGCDrawAllLoopP0ScreenRise, gNdsFighterGCDrawAllLoopP1ScreenRise',
        'printf "GCDRAWALL_MOVE=%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsFighterGCDrawAllLoopP0FloorYMilli, gNdsFighterGCDrawAllLoopP1FloorYMilli, gNdsFighterGCDrawAllLoopP0RootDeltaXMilli, gNdsFighterGCDrawAllLoopP1RootDeltaXMilli, gNdsFighterGCDrawAllLoopP0RootRiseMilli, gNdsFighterGCDrawAllLoopP1RootRiseMilli, gNdsFighterGCDrawAllLoopP0RootYFinalMilli, gNdsFighterGCDrawAllLoopP1RootYFinalMilli, gNdsFighterGCDrawAllLoopP0RootDirectionOK, gNdsFighterGCDrawAllLoopP1RootDirectionOK, gNdsFighterGCDrawAllLoopP0FloorOK, gNdsFighterGCDrawAllLoopP1FloorOK',
        'printf "GCDRAWALL_TRANS=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCDrawAllLoopFallDetectCount, gNdsFighterGCDrawAllLoopLandingDetectCount, gNdsFighterGCDrawAllLoopSetGroundCount, gNdsFighterGCDrawAllLoopSetAirCount, gNdsFighterGCDrawAllLoopWaitSetStatusCount, gNdsFighterGCDrawAllLoopRunBrakeEndCount, gNdsFighterGCDrawAllLoopJumpAnimEndCount, gNdsFighterGCDrawAllLoopLandingEndCount, gNdsFighterGCDrawAllLoopDeferredInterruptCheckCount',
        'printf "GCDRAWALL_SAFE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterGCDrawAllLoopGObjDelta, gNdsFighterGCDrawAllLoopUnexpectedStatusCount, gNdsFighterGCDrawAllLoopDeniedStatusCount, gNdsFighterGCDrawAllLoopProcessAttachEscapeCount, gNdsFighterGCDrawAllLoopDisplayProbeCount, gNdsFighterGCDrawAllLoopGameplayUpdateCount, gNdsFighterGCDrawAllLoopDrawCallCount, gNdsFighterGCDrawAllLoopMatrixCallCount, gNdsFighterGCDrawAllLoopRootYDriftCount, gNdsFighterGCDrawAllLoopGADriftCount',
        'printf "PLATFORM_DL_PREVIEW=%u,%u,%u,%u\n", gNdsOriginalDLPreviewReady, gNdsOriginalDLPreviewWidth, gNdsOriginalDLPreviewHeight, gNdsOriginalDLPreviewCommitCount',
        'printf "PLATFORM_HW=%u,%u\n", gNdsHardwareRendererSubmittedFrameCount, gNdsHardwareRendererFlushCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    if ($RequireStageMPPassiveRecoverLoop) {
        $dashRunCommands = @(
            'printf "DASH_RUN=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDashRunResult, gNdsFighterMarioFoxDashRunSafeResult, gNdsFighterMarioFoxDashRunMask, gNdsFighterMarioFoxDashRunDeferredMask, gNdsFighterMarioFoxDashRunCount',
            'printf "DASH_RUN_ATTACK_EVENT_POS=%#x,%u,%u,%u,%d,%d,%d,%d,%d,%#x\n", gNdsFighterDashRunAttackEventPositionMask, gNdsFighterDashRunAttackEventPositionState, gNdsFighterDashRunAttackEventPositionAttackID, gNdsFighterDashRunAttackEventPositionJointID, gNdsFighterDashRunAttackEventPositionX, gNdsFighterDashRunAttackEventPositionY, gNdsFighterDashRunAttackEventPositionZ, gNdsFighterDashRunAttackEventPositionMatrixFlag, gNdsFighterDashRunAttackEventPositionMatrixValue, gNdsFighterDashRunAttackEventAttackIDMask',
            'printf "DASH_RUN_PROCPARAMS=%#x,%d,%d,%d,%d,%d,%u,%u,%u\n", gNdsFighterDashRunProcParamsMask, gNdsFighterDashRunProcParamsDamageBefore, gNdsFighterDashRunProcParamsDamageAfter, gNdsFighterDashRunProcParamsQueueBefore, gNdsFighterDashRunProcParamsLagBefore, gNdsFighterDashRunProcParamsHitlag, gNdsFighterDashRunProcParamsPaused, gNdsFighterDashRunProcParamsStatusBefore, gNdsFighterDashRunProcParamsStatusAfter',
            'printf "DASH_RUN_PROCPARAMS_RUMBLE=%#x,%u,%u,%d\n", gNdsFighterDashRunProcParamsRumbleMask, gNdsFighterDashRunProcParamsRumbleCount, gNdsFighterDashRunProcParamsRumbleLastID, gNdsFighterDashRunProcParamsRumbleLastLength',
            'printf "DASH_RUN_DAMAGE_STATUS=%#x,%u,%u,%u,%u,%u\n", gNdsFighterDashRunDamageStatusMask, gNdsFighterDashRunDamageStatusLevel, gNdsFighterDashRunDamageStatusIndex, gNdsFighterDashRunDamageStatusGround, gNdsFighterDashRunDamageStatusAir, gNdsFighterDashRunDamageStatusElectric',
            'printf "DASH_RUN_DAMAGE_SETUP=%#x,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d\n", gNdsFighterDashRunDamageSetupMask, gNdsFighterDashRunDamageSetupStatusBefore, gNdsFighterDashRunDamageSetupStatusAfter, gNdsFighterDashRunDamageSetupMotionAfter, gNdsFighterDashRunDamageSetupGAAfter, gNdsFighterDashRunDamageSetupHitstunBefore, gNdsFighterDashRunDamageSetupHitstunAfter, gNdsFighterDashRunDamageSetupVelGroundMilli, gNdsFighterDashRunDamageSetupVelAirXMilli, gNdsFighterDashRunDamageSetupVelAirYMilli, gNdsFighterDashRunDamageSetupVelPhysicsMilli',
            'printf "DASH_RUN_DAMAGE_DUST=%#x,%#x\n", gNdsFighterDashRunDamageDustMask, gNdsFighterDashRunDamageDustWaits',
            'printf "DASH_RUN_DAMAGE_DUST_UPDATE=%#x,%u,%d\n", gNdsFighterDashRunDamageDustUpdateMask, gNdsFighterDashRunDamageDustUpdateEffectCount, gNdsFighterDashRunDamageDustUpdateWaitAfter',
            'printf "DASH_RUN_DAMAGE_HITSTUN_PUBLIC=%#x,%d,%d\n", gNdsFighterDashRunDamageHitstunPublicMask, gNdsFighterDashRunDamageHitstunPublicAfter, gNdsFighterDashRunDamageHitstunPublicKnockbackMilli',
            'printf "DASH_RUN_DAMAGE_COLANIM=%#x,%#x,%u\n", gNdsFighterDashRunDamageColAnimMask, gNdsFighterDashRunDamageColAnimIDs, gNdsFighterDashRunDamageColAnimCount',
            'printf "DASH_RUN_DAMAGE_COLANIM_UPDATE=%#x,%#x,%u\n", gNdsFighterDashRunDamageColAnimUpdateMask, gNdsFighterDashRunDamageColAnimUpdateIDs, gNdsFighterDashRunDamageColAnimUpdateCount',
            'printf "DASH_RUN_DAMAGE_INVINCIBLE=%#x,%d,%d\n", gNdsFighterDashRunDamageInvincibleMask, gNdsFighterDashRunDamageInvincibleTicsAfter, gNdsFighterDashRunDamageInvincibleHitStatusAfter',
            'printf "DASH_RUN_DAMAGE_LAGUPDATE=%#x,%d,%d\n", gNdsFighterDashRunDamageLagUpdateMask, gNdsFighterDashRunDamageLagUpdateDeltaXMilli, gNdsFighterDashRunDamageLagUpdateDeltaYMilli',
            'printf "DASH_RUN_DAMAGE_COMMON_PHYSICS=%#x,%d,%d,%d,%u\n", gNdsFighterDashRunDamageCommonPhysicsMask, gNdsFighterDashRunDamageCommonPhysicsGroundMilli, gNdsFighterDashRunDamageCommonPhysicsAirFrictionXMilli, gNdsFighterDashRunDamageCommonPhysicsAirDriftYMilli, gNdsFighterDashRunDamageCommonPhysicsClearState',
            'printf "DASH_RUN_DAMAGE_COMMON_CALLBACKS=%#x\n", gNdsFighterDashRunDamageCommonCallbackMask',
            'printf "DASH_RUN_DAMAGE_LEVELS=%#x\n", gNdsFighterDashRunDamageLevelMask',
            'printf "DASH_RUN_DAMAGE_HOLD_RESIST=%#x\n", gNdsFighterDashRunDamageHoldResistMask',
            'printf "DASH_RUN_DAMAGE_UPDATE_CATCH_RESIST=%#x\n", gNdsFighterDashRunDamageUpdateCatchResistMask',
            'printf "DASH_RUN_DAMAGE_AIR_MAP_WALL=%#x\n", gNdsFighterDashRunDamageAirMapWallMask',
            'printf "DASH_RUN_DAMAGE_KNOCKBACK_ANGLE=%#x\n", gNdsFighterDashRunDamageKnockbackAngleMask',
            'printf "DASH_RUN_DAMAGE_FALL_INTERRUPT=%#x\n", gNdsFighterDashRunDamageFallInterruptMask',
            'printf "DASH_RUN_DAMAGE_SCREEN_FLASH=%#x,%#x,%u\n", gNdsFighterDashRunDamageScreenFlashMask, gNdsFighterDashRunDamageScreenFlashIDs, gNdsFighterDashRunDamageScreenFlashCount',
            'printf "DASH_RUN_DAMAGE_PUBLIC=%#x,%d,%u\n", gNdsFighterDashRunDamagePublicMask, gNdsFighterDashRunDamagePublicKnockbackMilli, gNdsFighterDashRunDamagePublicForceCount',
            'printf "DASH_RUN_DAMAGE_VOICE=%#x,%u,%u,%u\n", gNdsFighterDashRunDamageVoiceMask, gNdsFighterDashRunDamageVoiceCount, gNdsFighterDashRunDamageVoiceThresholdFGM, gNdsFighterDashRunDamageVoiceForceFGM',
            'printf "DASH_RUN_DAMAGE_FLYTOP=%#x,%u,%u,%u\n", gNdsFighterDashRunDamageFlyTopMask, gNdsFighterDashRunDamageFlyTopStatus, gNdsFighterDashRunDamageFlyTopMotion, gNdsFighterDashRunDamageFlyTopAngle',
            'printf "DASH_RUN_DAMAGE_REPLACE_ELECTRIC=%#x,%u,%u,%u,%u,%u\n", gNdsFighterDashRunDamageReplaceElectricMask, gNdsFighterDashRunDamageReplaceElectricStatus, gNdsFighterDashRunDamageReplaceElectricStoredStatus, gNdsFighterDashRunDamageReplaceElectricMotion, gNdsFighterDashRunDamageReplaceElectricDispatchStatus, gNdsFighterDashRunDamageReplaceElectricDispatchMotion',
            'printf "DASH_RUN_DAMAGE_FLYROLL=%#x,%u,%u,%u\n", gNdsFighterDashRunDamageFlyRollMask, gNdsFighterDashRunDamageFlyRollStatus, gNdsFighterDashRunDamageFlyRollMotion, gNdsFighterDashRunDamageFlyRollPercent',
            'printf "DASH_RUN_DAMAGE_KIRBYCOPY=%#x,%u,%u,%u\n", gNdsFighterDashRunDamageKirbyCopyMask, gNdsFighterDashRunDamageKirbyCopyBefore, gNdsFighterDashRunDamageKirbyCopyAfter, gNdsFighterDashRunDamageKirbyCopyFGM',
            'printf "DASH_RUN_DAMAGE_ITEM_HEAVY=%#x\n", gNdsFighterDashRunDamageItemHeavyMask',
            'printf "DASH_RUN_DAMAGE_ITEM_BYPASS=%#x\n", gNdsFighterDashRunDamageItemBypassMask',
            'printf "DASH_RUN_DAMAGE_KIND=%#x\n", gNdsFighterDashRunDamageKindPreserveMask',
            'printf "DASH_RUN_DAMAGE_SLEEP=%#x,%u,%u,%u,%u\n", gNdsFighterDashRunDamageUpdateSleepMask, gNdsFighterDashRunDamageUpdateSleepStatusBefore, gNdsFighterDashRunDamageUpdateSleepStatusAfter, gNdsFighterDashRunDamageUpdateSleepMotionAfter, gNdsFighterDashRunDamageUpdateSleepColAnimDelta',
            'printf "DASH_RUN_DAMAGE_LOSEGRIP=%#x,%u,%u,%u,%d,%d,%u\n", gNdsFighterDashRunDamageLoseGripMask, gNdsFighterDashRunDamageLoseGripReleaseCount, gNdsFighterDashRunDamageLoseGripCollisionCount, gNdsFighterDashRunDamageLoseGripSetAirCount, gNdsFighterDashRunDamageLoseGripTargetX, gNdsFighterDashRunDamageLoseGripTargetY, gNdsFighterDashRunDamageLoseGripLinkClearCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $dashRunCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPDamageRecoverLoop) {
        $mpDamageRecoverCommands = @(
            'printf "STAGE_MPDAMAGE_RECOVER=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPDamageRecoverLoopResult, gNdsFighterMarioFoxStageMPDamageRecoverLoopSafeResult, gNdsFighterMarioFoxStageMPDamageRecoverLoopMask, gNdsFighterMarioFoxStageMPDamageRecoverLoopDeferredMask, gNdsFighterMarioFoxStageMPDamageRecoverLoopCount',
            'printf "STAGE_MPDAMAGE_RECOVER_SETUP=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPDamageRecoverLoopPrepared, gNdsStageMPDamageRecoverLoopBasePassiveRecoverSeen, gNdsStageMPDamageRecoverLoopBaseDashDamageSeen, gNdsStageMPDamageRecoverLoopContactSeedCount, gNdsStageMPDamageRecoverLoopContactDecisionCount, gNdsStageMPDamageRecoverLoopContactHitCount, gNdsStageMPDamageRecoverLoopProcParamsCallCount, gNdsStageMPDamageRecoverLoopProcParamsHitCount',
            'printf "STAGE_MPDAMAGE_RECOVER_CONTACT=%d,%d,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPDamageRecoverLoopAttackerSlot, gNdsStageMPDamageRecoverLoopVictimSlot, gNdsStageMPDamageRecoverLoopContactSeedCount, gNdsStageMPDamageRecoverLoopContactDecisionCount, gNdsStageMPDamageRecoverLoopContactHitCount, gNdsStageMPDamageRecoverLoopProcParamsCallCount, gNdsStageMPDamageRecoverLoopProcParamsHitCount, gNdsStageMPDamageRecoverLoopProcLagStartCount, gNdsStageMPDamageRecoverLoopProcLagUpdateCount, gNdsStageMPDamageRecoverLoopProcLagEndCount',
            'printf "STAGE_MPDAMAGE_RECOVER_DAMAGE=%u,%u,%u,%u,%u,%u,%d,%d,%u,%u\n", gNdsStageMPDamageRecoverLoopVictimDamageBefore, gNdsStageMPDamageRecoverLoopVictimDamageAfter, gNdsStageMPDamageRecoverLoopDamageQueueBefore, gNdsStageMPDamageRecoverLoopDamageQueueAfter, gNdsStageMPDamageRecoverLoopHitlagTics, gNdsStageMPDamageRecoverLoopHitstunStart, gNdsStageMPDamageRecoverLoopDamageKnockbackMilli, gNdsStageMPDamageRecoverLoopDamageAngle, gNdsStageMPDamageRecoverLoopDamageElement, gNdsStageMPDamageRecoverLoopDamageIndex',
            'printf "STAGE_MPDAMAGE_RECOVER_STATUS=%u,%u,%d,%u,%u,%d,%u,%d\n", gNdsStageMPDamageRecoverLoopExpectedGroundDamageStatus, gNdsStageMPDamageRecoverLoopActualGroundDamageStatus, gNdsStageMPDamageRecoverLoopActualGroundDamageMotion, gNdsStageMPDamageRecoverLoopExpectedAirDamageStatus, gNdsStageMPDamageRecoverLoopActualAirDamageStatus, gNdsStageMPDamageRecoverLoopActualAirDamageMotion, gNdsStageMPDamageRecoverLoopDamageFallStatusAfter, gNdsStageMPDamageRecoverLoopDamageFallMotionAfter',
            'printf "STAGE_MPDAMAGE_RECOVER_CALLBACKS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPDamageRecoverLoopDamageUpdateMainCallCount, gNdsStageMPDamageRecoverLoopDamageUpdateMainTailCount, gNdsStageMPDamageRecoverLoopGotoDamageStatusCount, gNdsStageMPDamageRecoverLoopSetStatusCallCount, gNdsStageMPDamageRecoverLoopDamageCommonUpdateCount, gNdsStageMPDamageRecoverLoopDamageCommonInterruptCount, gNdsStageMPDamageRecoverLoopDamageCommonPhysicsCount, gNdsStageMPDamageRecoverLoopDamageAirUpdateCount, gNdsStageMPDamageRecoverLoopDamageAirInterruptCount, gNdsStageMPDamageRecoverLoopDamageAirPhysicsCount',
            'printf "STAGE_MPDAMAGE_RECOVER_GROUND=%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPDamageRecoverLoopGroundProbeCount, gNdsStageMPDamageRecoverLoopGroundProbeHitCount, gNdsStageMPDamageRecoverLoopGroundWaitHandoffCount, gNdsStageMPDamageRecoverLoopDamageAirMapCount, gNdsStageMPDamageRecoverLoopDamageFallSetStatusCount, gNdsStageMPDamageRecoverLoopDamageFallMapCount, gNdsStageMPDamageRecoverLoopDamageFallStatusAfter',
            'printf "STAGE_MPDAMAGE_RECOVER_BRANCHES=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPDamageRecoverLoopPassiveStandProbeCount, gNdsStageMPDamageRecoverLoopPassiveStandHitCount, gNdsStageMPDamageRecoverLoopPassiveStandWaitHandoffCount, gNdsStageMPDamageRecoverLoopPassiveProbeCount, gNdsStageMPDamageRecoverLoopPassiveHitCount, gNdsStageMPDamageRecoverLoopPassiveWaitHandoffCount, gNdsStageMPDamageRecoverLoopDownBounceProbeCount, gNdsStageMPDamageRecoverLoopDownBounceHitCount, gNdsStageMPDamageRecoverLoopGroundWaitHandoffCount',
            'printf "STAGE_MPDAMAGE_RECOVER_VEL=%d,%d,%d,%d,%d,%d,%d\n", gNdsStageMPDamageRecoverLoopVictimVelXDamageMilli, gNdsStageMPDamageRecoverLoopVictimVelYDamageMilli, gNdsStageMPDamageRecoverLoopVictimVelGroundMilli, gNdsStageMPDamageRecoverLoopVictimRootXBeforeMilli, gNdsStageMPDamageRecoverLoopVictimRootYBeforeMilli, gNdsStageMPDamageRecoverLoopVictimRootXAfterMilli, gNdsStageMPDamageRecoverLoopVictimRootYAfterMilli',
            'printf "STAGE_MPDAMAGE_RECOVER_SAFE=%d,%d,%u,%u,%u,%u,%u\n", gNdsStageMPDamageRecoverLoopP0FinalLineID, gNdsStageMPDamageRecoverLoopP1FinalLineID, gNdsStageMPDamageRecoverLoopP0FloorOK, gNdsStageMPDamageRecoverLoopP1FloorOK, gNdsStageMPDamageRecoverLoopNoFinalRecenterCount, gNdsStageMPDamageRecoverLoopUnexpectedSceneCount, gNdsStageMPDamageRecoverLoopUnsafeCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpDamageRecoverCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPLiveHitDamageLoop) {
        $mpLiveHitDamageCommands = @(
            'printf "STAGE_MPLIVEHIT_DAMAGE=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPLiveHitDamageLoopResult, gNdsFighterMarioFoxStageMPLiveHitDamageLoopSafeResult, gNdsFighterMarioFoxStageMPLiveHitDamageLoopMask, gNdsFighterMarioFoxStageMPLiveHitDamageLoopDeferredMask, gNdsFighterMarioFoxStageMPLiveHitDamageLoopCount',
            'printf "STAGE_MPLIVEHIT_SETUP=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPLiveHitDamageLoopPrepared, gNdsStageMPLiveHitDamageLoopBaseDamageRecoverSeen, gNdsStageMPLiveHitDamageLoopBaseDashDamageSeen, gNdsStageMPLiveHitDamageLoopAttackerSlot, gNdsStageMPLiveHitDamageLoopVictimSlot, gNdsStageMPLiveHitDamageLoopStateSavedCount, gNdsStageMPLiveHitDamageLoopStateRestoredCount, gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount',
            'printf "STAGE_MPLIVEHIT_ATTACK=%u,%u,%d,%u,%#x\n", gNdsStageMPLiveHitDamageLoopAttackSeedCount, gNdsStageMPLiveHitDamageLoopAttack12StatusAfter, gNdsStageMPLiveHitDamageLoopAttack12MotionAfter, gNdsStageMPLiveHitDamageLoopAttack12GAAfter, gNdsStageMPLiveHitDamageLoopAttack12CallbackMask',
            'printf "STAGE_MPLIVEHIT_EVENTS=%u,%u,%u,%#x,%#x\n", gNdsStageMPLiveHitDamageLoopAnimEventsCallCount, gNdsStageMPLiveHitDamageLoopAnimEventsSelectedScriptCount, gNdsStageMPLiveHitDamageLoopAnimEventsMakeAttackCount, gNdsStageMPLiveHitDamageLoopAnimEventsCommandMask, gNdsFighterDashRunAttackEventRecordCarryMask',
            'printf "STAGE_MPLIVEHIT_ATTACKDATA=%u,%u,%u,%d,%d,%d,%d,%d,%#x\n", gNdsStageMPLiveHitDamageLoopAttackID, gNdsStageMPLiveHitDamageLoopAttackGroupID, gNdsStageMPLiveHitDamageLoopAttackJointID, gNdsStageMPLiveHitDamageLoopAttackDamage, gNdsStageMPLiveHitDamageLoopAttackSizeRaw, gNdsStageMPLiveHitDamageLoopAttackSize, gNdsStageMPLiveHitDamageLoopAttackAngle, gNdsStageMPLiveHitDamageLoopAttackKBG, gNdsStageMPLiveHitDamageLoopAttackFlags',
            'printf "STAGE_MPLIVEHIT_SECONDARY=%#x,%u,%u,%d,%d,%d,%d,%#x\n", gNdsStageMPLiveHitDamageLoopSecondaryMask, gNdsStageMPLiveHitDamageLoopSecondaryAttackID, gNdsStageMPLiveHitDamageLoopSecondaryJointID, gNdsStageMPLiveHitDamageLoopSecondaryDamage, gNdsStageMPLiveHitDamageLoopSecondarySize, gNdsStageMPLiveHitDamageLoopSecondaryOffsetX, gNdsStageMPLiveHitDamageLoopSecondaryAngle, gNdsStageMPLiveHitDamageLoopSecondaryFlags',
            'printf "STAGE_MPLIVEHIT_HURTBOX=%#x,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPLiveHitDamageLoopHurtboxMask, gNdsStageMPLiveHitDamageLoopHurtboxActiveCount, gNdsStageMPLiveHitDamageLoopHurtboxNoneStopSlot, gNdsStageMPLiveHitDamageLoopHurtboxIntangibleSkipCount, gNdsStageMPLiveHitDamageLoopHurtboxTestCount, gNdsStageMPLiveHitDamageLoopHurtboxHitCount, gNdsStageMPLiveHitDamageLoopHurtboxFirstHitSlot, gNdsStageMPLiveHitDamageLoopHurtboxFirstHitJoint, gNdsStageMPLiveHitDamageLoopHurtboxFirstHitStatus',
            'printf "STAGE_MPLIVEHIT_HURTBOX_DAMAGE=%#x,%u,%u,%d,%d,%d,%d,%u\n", gNdsStageMPLiveHitDamageLoopHurtboxDamageMask, gNdsStageMPLiveHitDamageLoopHurtboxDamageSlot, gNdsStageMPLiveHitDamageLoopHurtboxDamageJoint, gNdsStageMPLiveHitDamageLoopHurtboxDamageQueueBefore, gNdsStageMPLiveHitDamageLoopHurtboxDamageQueueAfter, gNdsStageMPLiveHitDamageLoopHurtboxDamagePercentBefore, gNdsStageMPLiveHitDamageLoopHurtboxDamagePercentAfter, gNdsStageMPLiveHitDamageLoopHurtboxDamageHitlag',
            'printf "STAGE_MPLIVEHIT_EFFECTONLY=%#x,%u,%d,%d,%d,%d,%d,%u,%u,%u,%u,%d\n", gNdsStageMPLiveHitDamageLoopEffectOnlyMask, gNdsStageMPLiveHitDamageLoopEffectOnlyStatus, gNdsStageMPLiveHitDamageLoopEffectOnlyDamage, gNdsStageMPLiveHitDamageLoopEffectOnlyQueueBefore, gNdsStageMPLiveHitDamageLoopEffectOnlyQueueAfter, gNdsStageMPLiveHitDamageLoopEffectOnlyPercentBefore, gNdsStageMPLiveHitDamageLoopEffectOnlyPercentAfter, gNdsStageMPLiveHitDamageLoopEffectOnlyHitLogBefore, gNdsStageMPLiveHitDamageLoopEffectOnlyHitLogAfter, gNdsStageMPLiveHitDamageLoopEffectOnlyEffectCount, gNdsStageMPLiveHitDamageLoopEffectOnlySFXCount, gNdsStageMPLiveHitDamageLoopEffectOnlyAttackDamageAfter',
            'printf "STAGE_MPLIVEHIT_RESIST=%#x,%d,%d,%d,%u,%d,%d,%d,%d,%u,%u,%u,%u,%d\n", gNdsStageMPLiveHitDamageLoopDamageResistMask, gNdsStageMPLiveHitDamageLoopDamageResistDamage, gNdsStageMPLiveHitDamageLoopDamageResistBefore, gNdsStageMPLiveHitDamageLoopDamageResistAfter, gNdsStageMPLiveHitDamageLoopDamageResistFlagAfter, gNdsStageMPLiveHitDamageLoopDamageResistQueueBefore, gNdsStageMPLiveHitDamageLoopDamageResistQueueAfter, gNdsStageMPLiveHitDamageLoopDamageResistPercentBefore, gNdsStageMPLiveHitDamageLoopDamageResistPercentAfter, gNdsStageMPLiveHitDamageLoopDamageResistHitLogBefore, gNdsStageMPLiveHitDamageLoopDamageResistHitLogAfter, gNdsStageMPLiveHitDamageLoopDamageResistEffectCount, gNdsStageMPLiveHitDamageLoopDamageResistSFXCount, gNdsStageMPLiveHitDamageLoopDamageResistAttackDamageAfter',
            'printf "STAGE_MPLIVEHIT_RESIST_BREAK=%#x,%d,%d,%u,%d,%d,%d\n", gNdsStageMPLiveHitDamageLoopDamageResistBreakMask, gNdsStageMPLiveHitDamageLoopDamageResistBreakBefore, gNdsStageMPLiveHitDamageLoopDamageResistBreakAfter, gNdsStageMPLiveHitDamageLoopDamageResistBreakFlagAfter, gNdsStageMPLiveHitDamageLoopDamageResistBreakDamageAfter, gNdsStageMPLiveHitDamageLoopDamageResistBreakQueueAfter, gNdsStageMPLiveHitDamageLoopDamageResistBreakLagAfter',
            'printf "STAGE_MPLIVEHIT_THROWATTR=%#x,%u,%d,%u,%d,%u,%d\n", gNdsStageMPLiveHitDamageLoopThrowAttribMask, gNdsStageMPLiveHitDamageLoopThrowAttribDirectPlayer, gNdsStageMPLiveHitDamageLoopThrowAttribDirectPlayerNum, gNdsStageMPLiveHitDamageLoopThrowAttribOwnerPlayer, gNdsStageMPLiveHitDamageLoopThrowAttribOwnerPlayerNum, gNdsStageMPLiveHitDamageLoopThrowAttribHitLogPlayer, gNdsStageMPLiveHitDamageLoopThrowAttribHitLogPlayerNum',
            'printf "STAGE_MPLIVEHIT_ATTACK_CLASH=%#x,%u,%u,%d,%d,%d,%d,%d,%d,%u\n", gNdsStageMPLiveHitDamageLoopAttackClashMask, gNdsStageMPLiveHitDamageLoopAttackClashThisGroup, gNdsStageMPLiveHitDamageLoopAttackClashOtherGroup, gNdsStageMPLiveHitDamageLoopAttackClashThisPush, gNdsStageMPLiveHitDamageLoopAttackClashOtherPush, gNdsStageMPLiveHitDamageLoopAttackClashThisReboundMilli, gNdsStageMPLiveHitDamageLoopAttackClashOtherReboundMilli, gNdsStageMPLiveHitDamageLoopAttackClashThisLR, gNdsStageMPLiveHitDamageLoopAttackClashOtherLR, gNdsStageMPLiveHitDamageLoopAttackClashEffectCount',
            'printf "STAGE_MPLIVEHIT_CATCHSTAT=%#x,%d,%d,%d,%u,%u\n", gNdsStageMPLiveHitDamageLoopCatchStatMask, gNdsStageMPLiveHitDamageLoopCatchStatDistMilli, gNdsStageMPLiveHitDamageLoopCatchStatBeforeMilli, gNdsStageMPLiveHitDamageLoopCatchStatAfterMilli, gNdsStageMPLiveHitDamageLoopCatchStatSearchSet, gNdsStageMPLiveHitDamageLoopCatchStatRecordHurt',
            'printf "STAGE_MPLIVEHIT_CATCHSEARCH=%#x,%#x,%u,%u,%d\n", gNdsStageMPLiveHitDamageLoopCatchSearchMask, gNdsStageMPLiveHitDamageLoopCatchSearchSkipMask, gNdsStageMPLiveHitDamageLoopCatchSearchSlot, gNdsStageMPLiveHitDamageLoopCatchSearchJoint, gNdsStageMPLiveHitDamageLoopCatchSearchDistMilli',
            'printf "STAGE_MPLIVEHIT_POS=%d,%d,%d,%u,%u,%u,%u,%u\n", gNdsStageMPLiveHitDamageLoopAttackPosX, gNdsStageMPLiveHitDamageLoopAttackPosY, gNdsStageMPLiveHitDamageLoopAttackPosZ, gNdsStageMPLiveHitDamageLoopAttackStateBefore, gNdsStageMPLiveHitDamageLoopAttackStateAfterNew, gNdsStageMPLiveHitDamageLoopAttackStateAfterTransfer, gNdsStageMPLiveHitDamageLoopAttackStateAfterInterpolate, gNdsStageMPLiveHitDamageLoopAttackMatrixReset',
            'printf "STAGE_MPLIVEHIT_COLL=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPLiveHitDamageLoopRangeCallCount, gNdsStageMPLiveHitDamageLoopRangeHitCount, gNdsStageMPLiveHitDamageLoopRectangleCallCount, gNdsStageMPLiveHitDamageLoopRectangleHitCount, gNdsStageMPLiveHitDamageLoopCollisionDecisionCount, gNdsStageMPLiveHitDamageLoopCollisionHitCount, gNdsStageMPLiveHitDamageLoopDamageRecordInsertCount, gNdsStageMPLiveHitDamageLoopRepeatHitProbeCount, gNdsStageMPLiveHitDamageLoopRepeatHitRejectedCount, gNdsStageMPLiveHitDamageLoopAttackWritebackCount',
            'printf "STAGE_MPLIVEHIT_REHIT=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x\n", gNdsStageMPLiveHitDamageLoopHitInteractDamageCount, gNdsStageMPLiveHitDamageLoopHitInteractShieldCount, gNdsStageMPLiveHitDamageLoopHitInteractAttackCount, gNdsStageMPLiveHitDamageLoopHitInteractDamageDetectClear, gNdsStageMPLiveHitDamageLoopHitInteractAttackDetectClear, gNdsStageMPLiveHitDamageLoopHitInteractGroupAfterAttack, gNdsStageMPLiveHitDamageLoopRehitTimerSeed, gNdsStageMPLiveHitDamageLoopRehitTimerAfterRefresh, gNdsStageMPLiveHitDamageLoopRefreshStateAfter, gNdsStageMPLiveHitDamageLoopRefreshClearCount, gNdsStageMPLiveHitDamageLoopHitInteractDetectGateMask',
            'printf "STAGE_MPLIVEHIT_SHIELD=%u,%d,%d,%d,%d,%d,%d,%u,%d,%u,%d,%d,%#x\n", gNdsStageMPLiveHitDamageLoopShieldStatCount, gNdsStageMPLiveHitDamageLoopShieldAttackPushAfter, gNdsStageMPLiveHitDamageLoopShieldDamageBefore, gNdsStageMPLiveHitDamageLoopShieldDamageAfter, gNdsStageMPLiveHitDamageLoopShieldDamageTotalAfter, gNdsStageMPLiveHitDamageLoopShieldLR, gNdsStageMPLiveHitDamageLoopShieldPlayer, gNdsStageMPLiveHitDamageLoopShieldEffectCount, gNdsStageMPLiveHitDamageLoopShieldEffectSize, gNdsStageMPLiveHitDamageLoopShieldSetOffStatusAfter, gNdsStageMPLiveHitDamageLoopShieldSetOffMotionAfter, gNdsStageMPLiveHitDamageLoopShieldSetOffHitlag, gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask',
            'printf "STAGE_MPLIVEHIT_SHIELD_SETOFF_TICK=%#x,%u,%u,%d\n", gNdsStageMPLiveHitDamageLoopShieldSetOffTickMask, gNdsStageMPLiveHitDamageLoopShieldSetOffTickStatusHeld, gNdsStageMPLiveHitDamageLoopShieldSetOffTickStatusRelease, gNdsStageMPLiveHitDamageLoopShieldSetOffTickFramesMilli',
            'printf "STAGE_MPLIVEHIT_SHIELD_CONTACT=%#x,%u,%u,%u,%u,%d\n", gNdsStageMPLiveHitDamageLoopShieldContactMask, gNdsStageMPLiveHitDamageLoopShieldContactAttackID, gNdsStageMPLiveHitDamageLoopShieldContactDetectBefore, gNdsStageMPLiveHitDamageLoopShieldContactCollisionCount, gNdsStageMPLiveHitDamageLoopShieldContactHitCount, gNdsStageMPLiveHitDamageLoopShieldContactAngleMilli',
            'printf "STAGE_MPLIVEHIT_ORIG_REHIT=%u,%u,%u,%u,%u,%#x,%#x,%u,%#x,%#x\n", gNdsStageMPLiveHitDamageLoopOriginalRehitCallCount, gNdsStageMPLiveHitDamageLoopOriginalRehitFKind, gNdsStageMPLiveHitDamageLoopOriginalRehitTimerBefore, gNdsStageMPLiveHitDamageLoopOriginalRehitTimerMid, gNdsStageMPLiveHitDamageLoopOriginalRehitTimerAfter, gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshStateMask, gNdsStageMPLiveHitDamageLoopOriginalRehitRecordClearMask, gNdsStageMPLiveHitDamageLoopOriginalRehitAttackActiveAfter, gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask, gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshIDMask',
            'printf "STAGE_MPLIVEHIT_ORIG_REHIT_HIT=%u,%u,%d,%u,%u,%d,%#x,%d\n", gNdsStageMPLiveHitDamageLoopOriginalRehitHitCallCount, gNdsStageMPLiveHitDamageLoopOriginalRehitHitTimerAfter, gNdsStageMPLiveHitDamageLoopOriginalRehitHitVelYMilli, gNdsStageMPLiveHitDamageLoopOriginalRehitHitFastFallAfter, gNdsStageMPLiveHitDamageLoopOriginalRehitHitStatusAfter, gNdsStageMPLiveHitDamageLoopOriginalRehitHitMotionAfter, gNdsStageMPLiveHitDamageLoopOriginalRehitHitClearMask, gNdsStageMPLiveHitDamageLoopOriginalRehitHitAnimFrameMilli',
            'printf "STAGE_MPLIVEHIT_DAMAGESTATE=%u,%u,%u,%u,%u,%d,%d,%d\n", gNdsStageMPLiveHitDamageLoopVictimDamageBefore, gNdsStageMPLiveHitDamageLoopVictimDamageAfter, gNdsStageMPLiveHitDamageLoopVictimHitlagTics, gNdsStageMPLiveHitDamageLoopVictimHitstunBefore, gNdsStageMPLiveHitDamageLoopVictimHitstunAfter, gNdsStageMPLiveHitDamageLoopVictimKnockbackMilli, gNdsStageMPLiveHitDamageLoopVictimVelXDamageMilli, gNdsStageMPLiveHitDamageLoopVictimVelYDamageMilli',
            'printf "STAGE_MPLIVEHIT_PROC=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPLiveHitDamageLoopHitLogCount, gNdsStageMPLiveHitDamageLoopHitSFXCount, gNdsStageMPLiveHitDamageLoopHitStatsCount, gNdsStageMPLiveHitDamageLoopProcParamsCallCount, gNdsStageMPLiveHitDamageLoopProcParamsHitCount, gNdsStageMPLiveHitDamageLoopProcLagStartCount, gNdsStageMPLiveHitDamageLoopProcLagUpdateCount, gNdsStageMPLiveHitDamageLoopProcLagEndCount',
            'printf "STAGE_MPLIVEHIT_RECOVER=%u,%u,%d,%u,%u,%u\n", gNdsStageMPLiveHitDamageLoopDamageRecoverConsumed, gNdsStageMPLiveHitDamageLoopDamageRecoverStatusAfter, gNdsStageMPLiveHitDamageLoopDamageRecoverMotionAfter, gNdsStageMPLiveHitDamageLoopDamageRecoverPassiveStandHit, gNdsStageMPLiveHitDamageLoopDamageRecoverPassiveHit, gNdsStageMPLiveHitDamageLoopDamageRecoverDownBounceHit',
            'printf "STAGE_MPLIVEHIT_SAFE=%d,%d,%u,%u,%u,%u,%u,%u\n", gNdsStageMPLiveHitDamageLoopP0FinalLineID, gNdsStageMPLiveHitDamageLoopP1FinalLineID, gNdsStageMPLiveHitDamageLoopP0FloorOK, gNdsStageMPLiveHitDamageLoopP1FloorOK, gNdsStageMPLiveHitDamageLoopNoFinalRecenterCount, gNdsStageMPLiveHitDamageLoopUnexpectedSceneCount, gNdsStageMPLiveHitDamageLoopUnexpectedStatusCount, gNdsStageMPLiveHitDamageLoopUnsafeCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpLiveHitDamageCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPLiveHitStatusLoop) {
        $mpLiveHitStatusCommands = @(
            'printf "STAGE_MPLIVEHIT_STATUS=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPLiveHitStatusLoopResult, gNdsFighterMarioFoxStageMPLiveHitStatusLoopSafeResult, gNdsFighterMarioFoxStageMPLiveHitStatusLoopMask, gNdsFighterMarioFoxStageMPLiveHitStatusLoopDeferredMask, gNdsFighterMarioFoxStageMPLiveHitStatusLoopCount',
            'printf "STAGE_MPLIVEHIT_STATUS_SETUP=%u,%u,%u,%u\n", gNdsStageMPLiveHitStatusLoopBaseDamageSeen, gNdsStageMPLiveHitStatusLoopAttackerSlot, gNdsStageMPLiveHitStatusLoopVictimSlot, gNdsStageMPLiveHitDamageLoopAttackSeedCount',
            'printf "STAGE_MPLIVEHIT_STATUS_SEARCH=%#x,%#x,%u,%u,%u,%u\n", gNdsStageMPLiveHitStatusLoopSearchMask, gNdsStageMPLiveHitDamageLoopHurtboxDamageMask, gNdsStageMPLiveHitDamageLoopCollisionHitCount, gNdsStageMPLiveHitDamageLoopDamageRecordInsertCount, gNdsStageMPLiveHitDamageLoopHitInteractDamageCount, gNdsStageMPLiveHitDamageLoopHitInteractDamageDetectClear',
            'printf "STAGE_MPLIVEHIT_STATUS_PROC=%#x,%u,%u,%d,%u,%u,%u\n", gNdsStageMPLiveHitStatusLoopProcMask, gNdsStageMPLiveHitStatusLoopStatusBefore, gNdsStageMPLiveHitStatusLoopStatusAfter, gNdsStageMPLiveHitStatusLoopMotionAfter, gNdsStageMPLiveHitStatusLoopDamageBefore, gNdsStageMPLiveHitStatusLoopDamageAfter, gNdsStageMPLiveHitStatusLoopLagStartCount',
            'printf "STAGE_MPLIVEHIT_STATUS_HITLAG=%u,%u,%u,%u,%u\n", gNdsStageMPLiveHitStatusLoopHitlagStart, gNdsStageMPLiveHitStatusLoopHitlagEnd, gNdsStageMPLiveHitStatusLoopLagStartCount, gNdsStageMPLiveHitStatusLoopLagUpdateCount, gNdsStageMPLiveHitStatusLoopLagEndCount',
            'printf "STAGE_MPLIVEHIT_STATUS_CALLBACK=%#x,%u,%u,%u,%u,%u,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x\n", gNdsStageMPLiveHitStatusLoopCallbackMask, gNdsStageMPLiveHitStatusLoopCallbackStatus, gNdsStageMPLiveHitStatusLoopCallbackHitstunBefore, gNdsStageMPLiveHitStatusLoopCallbackHitstunAfter, gNdsStageMPLiveHitStatusLoopCallbackEndStatus, gNdsStageMPLiveHitStatusLoopCallbackEndMotion, gNdsStageMPLiveHitStatusLoopCallbackEndPublicKnockbackMilli, gNdsStageMPLiveHitStatusLoopCallbackPhysicsVelXMilli, gNdsStageMPLiveHitStatusLoopCallbackPhysicsVelYMilli, gNdsStageMPLiveHitStatusLoopCallbackInterruptCount, gNdsStageMPLiveHitStatusLoopCallbackMapCount, gNdsStageMPLiveHitStatusLoopCallbackMapNoCollisionCount, gNdsStageMPLiveHitStatusLoopCallbackMapCollisionCount, gNdsStageMPLiveHitStatusLoopCallbackMapPassiveStandCheckCount, gNdsStageMPLiveHitStatusLoopCallbackMapPassiveCheckCount, gNdsStageMPLiveHitStatusLoopCallbackMapDownBounceSetStatusCount, gNdsStageMPLiveHitStatusLoopCallbackMapWallMask, gNdsStageMPLiveHitStatusLoopCallbackMapCliffMask, gNdsStageMPLiveHitStatusLoopCallbackMapFallMask',
            'printf "STAGE_MPLIVEHIT_STATUS_REPEAT=%u,%u,%#x\n", gNdsStageMPLiveHitStatusLoopRepeatProbeCount, gNdsStageMPLiveHitStatusLoopRepeatRejectedCount, gNdsStageMPLiveHitStatusLoopDetectGateMask',
            'printf "STAGE_MPLIVEHIT_STATUS_SAFE=%d,%d,%u,%u,%u\n", gNdsStageMPLiveHitStatusLoopP0FinalLineID, gNdsStageMPLiveHitStatusLoopP1FinalLineID, gNdsStageMPLiveHitStatusLoopP0FloorOK, gNdsStageMPLiveHitStatusLoopP1FloorOK, gNdsStageMPLiveHitStatusLoopUnsafeCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpLiveHitStatusCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageDraw) {
        $stageCommands = @(
            'printf "STAGE_GCDRAWALL=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageGCDrawAllLoopResult, gNdsFighterMarioFoxStageGCDrawAllLoopSafeResult, gNdsFighterMarioFoxStageGCDrawAllLoopMask, gNdsFighterMarioFoxStageGCDrawAllLoopDeferredMask, gNdsFighterMarioFoxStageGCDrawAllLoopCount',
            'printf "STAGE_GCDRAWALL_CAPTURE=%u,%u,%#x,%#x,%u,%u\n", gNdsStageGCDrawAllLoopDrawAllCount, gNdsStageGCDrawAllLoopCameraCallbackCount, gNdsStageGCDrawAllLoopLayerCaptureMask, gNdsStageGCDrawAllLoopMapCaptureMask, gNdsStageGCDrawAllLoopCapturedDisplayCount, gNdsStageGCDrawAllLoopFighterDisplayCallbackCount',
            'printf "STAGE_GCDRAWALL_DOBJ=%u,%#x,%#x,%#x,%#x,%#x\n", gNdsStageGCDrawAllLoopDObjDrawCallbackCount, gNdsStageGCDrawAllLoopDObjDrawKindMask, gNdsStageGCDrawAllLoopLayerDObjMask, gNdsStageGCDrawAllLoopMapDObjMask, gNdsStageGCDrawAllLoopLayerDLReadyMask, gNdsStageGCDrawAllLoopMapDLReadyMask',
            'printf "STAGE_GCDRAWALL_SAFE=%u,%u,%u,%u,%u,%u\n", gNdsStageGCDrawAllLoopPrepared, gNdsStageGCDrawAllLoopBaseResultSeen, gNdsStageGCDrawAllLoopManualDisplayCallCount, gNdsStageGCDrawAllLoopUnexpectedSceneCount, gNdsStageGCDrawAllLoopNonStageCaptureCount, gNdsStageGCDrawAllLoopGObjCountDelta',
            'printf "STAGE_GCDRAWALL_PIXELS=%u,%u,%#x\n", gNdsStageGCDrawAllLoopPreviewCommitDelta, gNdsStageGCDrawAllLoopTotalPixelCount, gNdsStageGCDrawAllLoopCompatMask',
            'printf "STAGE_GCDRAWALL_HW=%u,%u\n", gNdsStageGCDrawAllLoopHardwareSubmitCount, gNdsStageGCDrawAllLoopHardwareTriangleCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $stageCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageCollision -and -not $RequireStageFloorEdge) {
        $collisionCommands = @(
            'printf "STAGE_COLLISION=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageCollisionLoopResult, gNdsFighterMarioFoxStageCollisionLoopSafeResult, gNdsFighterMarioFoxStageCollisionLoopMask, gNdsFighterMarioFoxStageCollisionLoopDeferredMask, gNdsFighterMarioFoxStageCollisionLoopCount',
            'printf "STAGE_COLLISION_GEOM=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageCollisionLoopPrepared, gNdsStageCollisionLoopBaseStageDrawSeen, gNdsStageCollisionLoopGroundDataReady, gNdsStageCollisionLoopGeometryReady, gNdsStageCollisionLoopYakumonoCount, gNdsStageCollisionLoopMapObjCount, gNdsStageCollisionLoopFloorLineCount, gNdsStageCollisionLoopTotalLineCount',
            'printf "STAGE_COLLISION_PROJECT=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageCollisionLoopProjectCallCount, gNdsStageCollisionLoopGeometryProjectCallCount, gNdsStageCollisionLoopLegacyFlatFallbackCount, gNdsStageCollisionLoopNoGeometryCount, gNdsStageCollisionLoopOutOfRangeLineCount, gNdsStageCollisionLoopBadVertexCount, gNdsStageCollisionLoopDivisionGuardCount, gNdsStageCollisionLoopProbeCount',
            'printf "STAGE_COLLISION_PROBES=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageCollisionLoopProbeHitCount, gNdsStageCollisionLoopProbeMissCount, gNdsStageCollisionLoopOffstageMissCount, gNdsStageCollisionLoopBelowFloorMissCount, gNdsStageCollisionLoopP0ProjectCount, gNdsStageCollisionLoopP1ProjectCount, gNdsStageCollisionLoopP0HitCount, gNdsStageCollisionLoopP1HitCount',
            'printf "STAGE_COLLISION_P0=%d,%d,%u,%d,%d,%d,%d,%d,%u\n", gNdsStageCollisionLoopP0FloorLineID, gNdsStageCollisionLoopP0FloorDistMilli, gNdsStageCollisionLoopP0FloorFlags, gNdsStageCollisionLoopP0FloorAngleX1000, gNdsStageCollisionLoopP0FloorAngleY1000, gNdsStageCollisionLoopP0RootYFinalMilli, gNdsStageCollisionLoopP0FloorYMilli, gNdsStageCollisionLoopP0EdgeRX, gNdsStageCollisionLoopP0FloorOK',
            'printf "STAGE_COLLISION_P1=%d,%d,%u,%d,%d,%d,%d,%d,%u\n", gNdsStageCollisionLoopP1FloorLineID, gNdsStageCollisionLoopP1FloorDistMilli, gNdsStageCollisionLoopP1FloorFlags, gNdsStageCollisionLoopP1FloorAngleX1000, gNdsStageCollisionLoopP1FloorAngleY1000, gNdsStageCollisionLoopP1RootYFinalMilli, gNdsStageCollisionLoopP1FloorYMilli, gNdsStageCollisionLoopP1EdgeRX, gNdsStageCollisionLoopP1FloorOK',
            'printf "STAGE_COLLISION_EDGE=%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsStageCollisionLoopP0EdgeLX, gNdsStageCollisionLoopP0EdgeLY, gNdsStageCollisionLoopP0EdgeRX, gNdsStageCollisionLoopP0EdgeRY, gNdsStageCollisionLoopP1EdgeLX, gNdsStageCollisionLoopP1EdgeLY, gNdsStageCollisionLoopP1EdgeRX, gNdsStageCollisionLoopP1EdgeRY',
            'printf "STAGE_COLLISION_KIND=%d,%u,%d,%d,%u,%u,%d,%u,%d,%u,%u,%u,%u\n", gNdsStageCollisionLoopFloorGroupID, gNdsStageCollisionLoopFloorGroupCount, gNdsStageCollisionLoopFloorLineMin, gNdsStageCollisionLoopFloorLineMaxExclusive, gNdsStageCollisionLoopP0FloorKind, gNdsStageCollisionLoopP1FloorKind, gNdsStageCollisionLoopP0FloorLineID, gNdsStageCollisionLoopP0FloorLineIsFloor, gNdsStageCollisionLoopP1FloorLineID, gNdsStageCollisionLoopP1FloorLineIsFloor, gNdsStageCollisionLoopNonFloorCandidateCount, gNdsStageCollisionLoopYakumonoDObjDeferredCount, gNdsStageCollisionLoopYakumonoDObjUnsafeIndexGuardCount',
            'printf "STAGE_COLLISION_SAFE=%u,%u,%u,%u,%u,%u\n", gNdsStageCollisionLoopP0FloorOK, gNdsStageCollisionLoopP1FloorOK, gNdsStageCollisionLoopGObjDelta, gNdsStageCollisionLoopUnexpectedSceneCount, gNdsStageCollisionLoopUnexpectedStatusCount, gNdsStageCollisionLoopUnsafeFallbackAfterPrepareCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $collisionCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageFloorFollow) {
        $floorFollowCommands = @(
            'printf "STAGE_FLOOR_FOLLOW=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageFloorFollowLoopResult, gNdsFighterMarioFoxStageFloorFollowLoopSafeResult, gNdsFighterMarioFoxStageFloorFollowLoopMask, gNdsFighterMarioFoxStageFloorFollowLoopDeferredMask, gNdsFighterMarioFoxStageFloorFollowLoopCount',
            'printf "STAGE_FLOOR_FOLLOW_SETUP=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageFloorFollowLoopPrepared, gNdsStageFloorFollowLoopBaseDrawSeen, gNdsStageFloorFollowLoopBaseCollisionSeen, gNdsStageFloorFollowLoopGeometryReady, gNdsStageFloorFollowLoopInitialSeedCount, gNdsStageFloorFollowLoopInitialAdoptCount, gNdsStageFloorFollowLoopFinalRecenterCount, gNdsStageFloorFollowLoopFinalAdoptCount',
            'printf "STAGE_FLOOR_FOLLOW_UPDATES=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageFloorFollowLoopMapUpdateCount, gNdsStageFloorFollowLoopP0MapUpdateCount, gNdsStageFloorFollowLoopP1MapUpdateCount, gNdsStageFloorFollowLoopProjectCallCount, gNdsStageFloorFollowLoopGeometryHitCount, gNdsStageFloorFollowLoopGeometryMissCount, gNdsStageFloorFollowLoopNoGeometryCount, gNdsStageFloorFollowLoopNonFloorLineCount, gNdsStageFloorFollowLoopClampCount, gNdsStageFloorFollowLoopNoClampCount',
            'printf "STAGE_FLOOR_FOLLOW_P0=%d,%u,%u,%d,%d,%d,%d,%d,%u,%#x,%u,%u\n", gNdsStageFloorFollowLoopP0FloorLineID, gNdsStageFloorFollowLoopP0FloorKind, gNdsStageFloorFollowLoopP0FloorLineIsFloor, gNdsStageFloorFollowLoopP0InitialRootXMilli, gNdsStageFloorFollowLoopP0FinalRootXMilli, gNdsStageFloorFollowLoopP0RootXDeltaMilli, gNdsStageFloorFollowLoopP0FinalRootYMilli, gNdsStageFloorFollowLoopP0FloorYMilli, gNdsStageFloorFollowLoopP0FloorOK, gNdsStageFloorFollowLoopP0FloorVisitMask, gNdsStageFloorFollowLoopP0StatusFinal, gNdsStageFloorFollowLoopP0GAFinal',
            'printf "STAGE_FLOOR_FOLLOW_P1=%d,%u,%u,%d,%d,%d,%d,%d,%u,%#x,%u,%u\n", gNdsStageFloorFollowLoopP1FloorLineID, gNdsStageFloorFollowLoopP1FloorKind, gNdsStageFloorFollowLoopP1FloorLineIsFloor, gNdsStageFloorFollowLoopP1InitialRootXMilli, gNdsStageFloorFollowLoopP1FinalRootXMilli, gNdsStageFloorFollowLoopP1RootXDeltaMilli, gNdsStageFloorFollowLoopP1FinalRootYMilli, gNdsStageFloorFollowLoopP1FloorYMilli, gNdsStageFloorFollowLoopP1FloorOK, gNdsStageFloorFollowLoopP1FloorVisitMask, gNdsStageFloorFollowLoopP1StatusFinal, gNdsStageFloorFollowLoopP1GAFinal',
            'printf "STAGE_FLOOR_FOLLOW_DRIFT=%d,%d,%d,%d,%d\n", gNdsStageFloorFollowLoopP0FinalDriftMilli, gNdsStageFloorFollowLoopP1FinalDriftMilli, gNdsStageFloorFollowLoopP0MaxDriftMilli, gNdsStageFloorFollowLoopP1MaxDriftMilli, gNdsStageFloorFollowLoopMaxDriftMilli'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $floorFollowCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageFloorEdge) {
        $floorEdgeCommands = @(
            'printf "STAGE_FLOOR_EDGE=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageFloorEdgeLoopResult, gNdsFighterMarioFoxStageFloorEdgeLoopSafeResult, gNdsFighterMarioFoxStageFloorEdgeLoopMask, gNdsFighterMarioFoxStageFloorEdgeLoopDeferredMask, gNdsFighterMarioFoxStageFloorEdgeLoopCount',
            'printf "STAGE_FLOOR_EDGE_LINE=%u,%u,%d,%d,%d,%d,%u,%u\n", gNdsStageFloorEdgeLoopPrepared, gNdsStageFloorEdgeLoopGeometryReady, gNdsStageFloorEdgeLoopSelectedLineID, gNdsStageFloorEdgeLoopLeftXMilli, gNdsStageFloorEdgeLoopRightXMilli, gNdsStageFloorEdgeLoopWidthMilli, gNdsStageFloorEdgeLoopSelectedLineKind, gNdsStageFloorEdgeLoopSelectedVertexCount',
            'printf "STAGE_FLOOR_EDGE_P0=%d,%d,%d,%d,%u,%u,%u,%#x\n", gNdsStageFloorEdgeLoopP0StartDistMilli, gNdsStageFloorEdgeLoopP0FinalDistMilli, gNdsStageFloorEdgeLoopP0DeltaDistMilli, gNdsStageFloorEdgeLoopP0MinDistMilli, gNdsStageFloorEdgeLoopP0ApproachOK, gNdsStageFloorEdgeLoopP0NearEdgeOK, gNdsStageFloorEdgeLoopP0FloorOK, gNdsStageFloorEdgeLoopP0FloorVisitMask',
            'printf "STAGE_FLOOR_EDGE_P1=%d,%d,%d,%d,%u,%u,%u,%#x\n", gNdsStageFloorEdgeLoopP1StartDistMilli, gNdsStageFloorEdgeLoopP1FinalDistMilli, gNdsStageFloorEdgeLoopP1DeltaDistMilli, gNdsStageFloorEdgeLoopP1MinDistMilli, gNdsStageFloorEdgeLoopP1ApproachOK, gNdsStageFloorEdgeLoopP1NearEdgeOK, gNdsStageFloorEdgeLoopP1FloorOK, gNdsStageFloorEdgeLoopP1FloorVisitMask',
            'printf "STAGE_FLOOR_EDGE_PROBES=%u,%u,%u,%u,%u\n", gNdsStageFloorEdgeLoopInsideProbeCount, gNdsStageFloorEdgeLoopInsideProbeHitCount, gNdsStageFloorEdgeLoopOutsideProbeCount, gNdsStageFloorEdgeLoopOutsideProbeMissCount, gNdsStageFloorEdgeLoopOutsideProbeUnexpectedHitCount',
            'printf "STAGE_FLOOR_EDGE_QUERIES=%u,%u,%u,%u,%u,%u,%u\n", gNdsStageFloorEdgeLoopFCCommonCallCount, gNdsStageFloorEdgeLoopFCCommonHitCount, gNdsStageFloorEdgeLoopLineTypeCallCount, gNdsStageFloorEdgeLoopVertexPositionCallCount, gNdsStageFloorEdgeLoopEdgeUnderLCallCount, gNdsStageFloorEdgeLoopEdgeUnderRCallCount, gNdsStageFloorEdgeLoopEdgeUnderDeferredCount',
            'printf "STAGE_FLOOR_EDGE_UPDATES=%u,%u,%u,%u,%u,%d,%d,%d\n", gNdsStageFloorEdgeLoopMapUpdateCount, gNdsStageFloorEdgeLoopP0MapUpdateCount, gNdsStageFloorEdgeLoopP1MapUpdateCount, gNdsStageFloorEdgeLoopPreClampDriftSampleCount, gNdsStageFloorEdgeLoopPreClampCount, gNdsStageFloorEdgeLoopP0MaxPreClampDriftMilli, gNdsStageFloorEdgeLoopP1MaxPreClampDriftMilli, gNdsStageFloorEdgeLoopMaxPreClampDriftMilli',
            'printf "STAGE_FLOOR_EDGE_SAFE=%u,%u,%u,%u,%u\n", gNdsStageFloorEdgeLoopFinalRecenterCount, gNdsStageFloorEdgeLoopFinalAdoptCount, gNdsStageFloorEdgeLoopUnexpectedSceneCount, gNdsStageFloorEdgeLoopUnexpectedStatusCount, gNdsStageFloorEdgeLoopUnsafeFallbackAfterPrepareCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $floorEdgeCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPProcessFloor) {
        $mpProcessFloorCommands = @(
            'printf "STAGE_MPPROCESS_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPProcessFloorLoopResult, gNdsFighterMarioFoxStageMPProcessFloorLoopSafeResult, gNdsFighterMarioFoxStageMPProcessFloorLoopMask, gNdsFighterMarioFoxStageMPProcessFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPProcessFloorLoopCount',
            'printf "STAGE_MPPROCESS_FLOOR_ADAPTER=%u,%u,%u,%u,%u\n", gNdsStageMPProcessFloorLoopPrepared, gNdsStageMPProcessFloorLoopBaseFloorEdgeSeen, gNdsStageMPProcessFloorLoopAdapterBuildCount, gNdsStageMPProcessFloorLoopAdapterCopyBackCount, gNdsStageMPProcessFloorLoopAdapterFallbackLRCount',
            'printf "STAGE_MPPROCESS_FLOOR_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPProcessFloorLoopProjectFloorIDCallCount, gNdsStageMPProcessFloorLoopProjectFloorIDHitCount, gNdsStageMPProcessFloorLoopProjectFloorIDMissCount, gNdsStageMPProcessFloorLoopTestNewCallCount, gNdsStageMPProcessFloorLoopTestNewHitCount, gNdsStageMPProcessFloorLoopTestNewMissCount, gNdsStageMPProcessFloorLoopTestNewEdgeBranchCount, gNdsStageMPProcessFloorLoopTestNewSetProjectCount, gNdsStageMPProcessFloorLoopSetLandingFloorCallCount, gNdsStageMPProcessFloorLoopSetCollideFloorCallCount',
            'printf "STAGE_MPPROCESS_FLOOR_FC=%u,%u,%u\n", gNdsStageMPProcessFloorLoopFCCommonPositiveDistCount, gNdsStageMPProcessFloorLoopFCCommonNegativeDistCount, gNdsStageMPProcessFloorLoopFCCommonZeroDistCount',
            'printf "STAGE_MPPROCESS_FLOOR_PROBES=%u,%u,%u,%u,%u,%u\n", gNdsStageMPProcessFloorLoopInsideProbeCount, gNdsStageMPProcessFloorLoopInsideProbeHitCount, gNdsStageMPProcessFloorLoopOutsideProbeCount, gNdsStageMPProcessFloorLoopOutsideProbeMissCount, gNdsStageMPProcessFloorLoopBelowFloorProbeCount, gNdsStageMPProcessFloorLoopBelowFloorPositiveDistCount',
            'printf "STAGE_MPPROCESS_FLOOR_P0=%u,%u,%u,%d,%u,%#x,%d,%d\n", gNdsStageMPProcessFloorLoopP0UpdateCount, gNdsStageMPProcessFloorLoopP0HitCount, gNdsStageMPProcessFloorLoopP0MissCount, gNdsStageMPProcessFloorLoopP0FinalLineID, gNdsStageMPProcessFloorLoopP0FinalLineIsFloor, gNdsStageMPProcessFloorLoopP0FinalMaskStat, gNdsStageMPProcessFloorLoopP0FinalDistMilli, gNdsStageMPProcessFloorLoopP0RootYMilli',
            'printf "STAGE_MPPROCESS_FLOOR_P1=%u,%u,%u,%d,%u,%#x,%d,%d\n", gNdsStageMPProcessFloorLoopP1UpdateCount, gNdsStageMPProcessFloorLoopP1HitCount, gNdsStageMPProcessFloorLoopP1MissCount, gNdsStageMPProcessFloorLoopP1FinalLineID, gNdsStageMPProcessFloorLoopP1FinalLineIsFloor, gNdsStageMPProcessFloorLoopP1FinalMaskStat, gNdsStageMPProcessFloorLoopP1FinalDistMilli, gNdsStageMPProcessFloorLoopP1RootYMilli',
            'printf "STAGE_MPPROCESS_FLOOR_SAFE=%u,%u,%u,%u\n", gNdsStageMPProcessFloorLoopNoFinalRecenterCount, gNdsStageMPProcessFloorLoopUnexpectedSceneCount, gNdsStageMPProcessFloorLoopUnexpectedStatusCount, gNdsStageMPProcessFloorLoopUnsafeCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpProcessFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPUpdateFloor) {
        $mpUpdateFloorCommands = @(
            'printf "STAGE_MPUPDATE_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPUpdateFloorLoopResult, gNdsFighterMarioFoxStageMPUpdateFloorLoopSafeResult, gNdsFighterMarioFoxStageMPUpdateFloorLoopMask, gNdsFighterMarioFoxStageMPUpdateFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPUpdateFloorLoopCount',
            'printf "STAGE_MPUPDATE_FLOOR_ADAPTER=%u,%u,%u,%u\n", gNdsStageMPUpdateFloorLoopPrepared, gNdsStageMPUpdateFloorLoopBaseMPProcessSeen, gNdsStageMPUpdateFloorLoopAdapterBuildCount, gNdsStageMPUpdateFloorLoopAdapterCopyBackCount',
            'printf "STAGE_MPUPDATE_FLOOR_UPDATE=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPUpdateFloorLoopUpdateMainCallCount, gNdsStageMPUpdateFloorLoopUpdateMainReturnTrueCount, gNdsStageMPUpdateFloorLoopUpdateMainReturnFalseCount, gNdsStageMPUpdateFloorLoopUpdateMainStepCount, gNdsStageMPUpdateFloorLoopUpdateMainMaxStepCount, gNdsStageMPUpdateFloorLoopUpdateMainSplitCount, gNdsStageMPUpdateFloorLoopUpdateMainCapCount, gNdsStageMPUpdateFloorLoopTranslateResetCount, gNdsStageMPUpdateFloorLoopProcCollCallCount',
            'printf "STAGE_MPUPDATE_FLOOR_COLL=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPUpdateFloorLoopAllCollisionsCallCount, gNdsStageMPUpdateFloorLoopAllCollisionsFloorHitCount, gNdsStageMPUpdateFloorLoopAllCollisionsFloorMissCount, gNdsStageMPUpdateFloorLoopAllCollisionsCliffEdgeBranchCount, gNdsStageMPUpdateFloorLoopAllCollisionsStopEdgeBranchCount, gNdsStageMPUpdateFloorLoopAllCollisionsDefaultEndCount, gNdsStageMPUpdateFloorLoopAllCollisionsWallDeferredCount, gNdsStageMPUpdateFloorLoopAllCollisionsCeilDeferredCount, gNdsStageMPUpdateFloorLoopAllCollisionsFloorEdgeAdjustDeferredCount, gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount',
            'printf "STAGE_MPUPDATE_FLOOR_CHECKS=%u,%u,%u,%u,%u,%u\n", gNdsStageMPUpdateFloorLoopCheckFloorCallCount, gNdsStageMPUpdateFloorLoopCheckCliffEdgeCallCount, gNdsStageMPUpdateFloorLoopCheckFloorHitCount, gNdsStageMPUpdateFloorLoopCheckCliffEdgeHitCount, gNdsStageMPUpdateFloorLoopCheckFloorMissCount, gNdsStageMPUpdateFloorLoopCheckCliffEdgeMissCount',
            'printf "STAGE_MPUPDATE_FLOOR_PROBES=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPUpdateFloorLoopInsideProbeCount, gNdsStageMPUpdateFloorLoopInsideProbeHitCount, gNdsStageMPUpdateFloorLoopOutsideProbeCount, gNdsStageMPUpdateFloorLoopOutsideProbeMissCount, gNdsStageMPUpdateFloorLoopBelowFloorProbeCount, gNdsStageMPUpdateFloorLoopBelowFloorHitCount, gNdsStageMPUpdateFloorLoopSplitProbeCount, gNdsStageMPUpdateFloorLoopSplitProbeStepCount',
            'printf "STAGE_MPUPDATE_FLOOR_P0=%u,%u,%u,%d,%d,%d,%d,%d,%#x,%u\n", gNdsStageMPUpdateFloorLoopP0UpdateCount, gNdsStageMPUpdateFloorLoopP0HitCount, gNdsStageMPUpdateFloorLoopP0MissCount, gNdsStageMPUpdateFloorLoopP0PosDiffXMilli, gNdsStageMPUpdateFloorLoopP0RootXBeforeMilli, gNdsStageMPUpdateFloorLoopP0RootXFinalMilli, gNdsStageMPUpdateFloorLoopP0RootYFinalMilli, gNdsStageMPUpdateFloorLoopP0FinalLineID, gNdsStageMPUpdateFloorLoopP0FinalMaskStat, gNdsStageMPUpdateFloorLoopP0FloorOK',
            'printf "STAGE_MPUPDATE_FLOOR_P1=%u,%u,%u,%d,%d,%d,%d,%d,%#x,%u\n", gNdsStageMPUpdateFloorLoopP1UpdateCount, gNdsStageMPUpdateFloorLoopP1HitCount, gNdsStageMPUpdateFloorLoopP1MissCount, gNdsStageMPUpdateFloorLoopP1PosDiffXMilli, gNdsStageMPUpdateFloorLoopP1RootXBeforeMilli, gNdsStageMPUpdateFloorLoopP1RootXFinalMilli, gNdsStageMPUpdateFloorLoopP1RootYFinalMilli, gNdsStageMPUpdateFloorLoopP1FinalLineID, gNdsStageMPUpdateFloorLoopP1FinalMaskStat, gNdsStageMPUpdateFloorLoopP1FloorOK',
            'printf "STAGE_MPUPDATE_FLOOR_SAFE=%u,%u,%u,%u,%u,%u\n", gNdsStageMPUpdateFloorLoopNoFinalRecenterCount, gNdsStageMPUpdateFloorLoopFallDeniedCount, gNdsStageMPUpdateFloorLoopOttottoDeniedCount, gNdsStageMPUpdateFloorLoopUnexpectedSceneCount, gNdsStageMPUpdateFloorLoopUnexpectedStatusCount, gNdsStageMPUpdateFloorLoopUnsafeCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpUpdateFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPSweepFloor) {
        $mpSweepFloorCommands = @(
            'printf "STAGE_MPSWEEP_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPSweepFloorLoopResult, gNdsFighterMarioFoxStageMPSweepFloorLoopSafeResult, gNdsFighterMarioFoxStageMPSweepFloorLoopMask, gNdsFighterMarioFoxStageMPSweepFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPSweepFloorLoopCount',
            'printf "STAGE_MPSWEEP_FLOOR_SETUP=%u,%u,%d,%d,%u\n", gNdsStageMPSweepFloorLoopPrepared, gNdsStageMPSweepFloorLoopBaseMPUpdateSeen, gNdsStageMPSweepFloorLoopProbeLineID, gNdsStageMPSweepFloorLoopAltLineID, gNdsStageMPSweepFloorLoopUnsafeCount',
            'printf "STAGE_MPSWEEP_FLOOR_CHECK=%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPSweepFloorLoopCheckFloorCallCount, gNdsStageMPSweepFloorLoopCheckFloorHitCount, gNdsStageMPSweepFloorLoopCheckFloorMissCount, gNdsStageMPSweepFloorLoopLineSweepRejectSameLineCount, gNdsStageMPSweepFloorLoopLineSweepAcceptNewLineCount, gNdsStageMPSweepFloorLoopMaskCurrFloorCount, gNdsStageMPUpdateFloorLoopAllCollisionsSecondFloorTestDeferredCount',
            'printf "STAGE_MPSWEEP_FLOOR_SWEEP=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPSweepFloorLoopLineSweepSameCallCount, gNdsStageMPSweepFloorLoopLineSweepSameHitCount, gNdsStageMPSweepFloorLoopLineSweepSameMissCount, gNdsStageMPSweepFloorLoopLineSweepDiffCallCount, gNdsStageMPSweepFloorLoopLineSweepDiffHitCount, gNdsStageMPSweepFloorLoopLineSweepDiffMissCount, gNdsStageMPSweepFloorLoopLineSweepVisitCount, gNdsStageMPSweepFloorLoopLineSweepCandidateCount',
            'printf "STAGE_MPSWEEP_FLOOR_SECOND=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPSweepFloorLoopSecondFloorCallCount, gNdsStageMPSweepFloorLoopSecondFloorHitCount, gNdsStageMPSweepFloorLoopSecondFloorMissCount, gNdsStageMPSweepFloorLoopLandingFloorCallCount, gNdsStageMPSweepFloorLoopFloorEdgeAdjustCallCount, gNdsStageMPSweepFloorLoopFloorEdgeAdjustDeferredCount, gNdsStageMPSweepFloorLoopMaskStatFloorEdgeClearCount, gNdsStageMPSweepFloorLoopIsCollEndClearCount',
            'printf "STAGE_MPSWEEP_FLOOR_PROBES=%u,%u,%u,%u,%u,%u\n", gNdsStageMPSweepFloorLoopSameLineProbeCount, gNdsStageMPSweepFloorLoopSameLineProbeHitCount, gNdsStageMPSweepFloorLoopDiffLineProbeCount, gNdsStageMPSweepFloorLoopDiffLineProbeHitCount, gNdsStageMPSweepFloorLoopNoHitProbeCount, gNdsStageMPSweepFloorLoopNoHitProbeMissCount',
            'printf "STAGE_MPSWEEP_FLOOR_P0=%d,%u,%u\n", gNdsStageMPSweepFloorLoopP0FinalLineID, gNdsStageMPSweepFloorLoopP0FinalLineIsFloor, gNdsStageMPSweepFloorLoopP0FloorOK',
            'printf "STAGE_MPSWEEP_FLOOR_P1=%d,%u,%u\n", gNdsStageMPSweepFloorLoopP1FinalLineID, gNdsStageMPSweepFloorLoopP1FinalLineIsFloor, gNdsStageMPSweepFloorLoopP1FloorOK'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpSweepFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCrossFloor) {
        $mpCrossFloorCommands = @(
            'printf "STAGE_MPCROSS_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCrossFloorLoopResult, gNdsFighterMarioFoxStageMPCrossFloorLoopSafeResult, gNdsFighterMarioFoxStageMPCrossFloorLoopMask, gNdsFighterMarioFoxStageMPCrossFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPCrossFloorLoopCount',
            'printf "STAGE_MPCROSS_FLOOR_SETUP=%u,%u,%u,%u,%u,%d,%d,%d,%d,%u\n", gNdsStageMPCrossFloorLoopPrepared, gNdsStageMPCrossFloorLoopBaseMPSweepSeen, gNdsStageMPCrossFloorLoopPrimeAttemptCount, gNdsStageMPCrossFloorLoopPrimeHitCount, gNdsStageMPCrossFloorLoopPrimeMissCount, gNdsStageMPCrossFloorLoopSourceLineID, gNdsStageMPCrossFloorLoopTargetLineID, gNdsStageMPCrossFloorLoopTargetXMilli, gNdsStageMPCrossFloorLoopTargetYMilli, gNdsStageMPCrossFloorLoopUnsafeCount',
            'printf "STAGE_MPCROSS_FLOOR_LIVE=%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCrossFloorLoopLiveSecondFloorCallCount, gNdsStageMPCrossFloorLoopLiveSecondFloorHitCount, gNdsStageMPCrossFloorLoopLiveSecondFloorMissCount, gNdsStageMPCrossFloorLoopLiveAcceptedNewLineCount, gNdsStageMPCrossFloorLoopLiveLandingFloorCount, gNdsStageMPCrossFloorLoopLiveFloorEdgeAdjustCount, gNdsStageMPCrossFloorLoopLiveCollEndClearCount',
            'printf "STAGE_MPCROSS_FLOOR_P0=%u,%d,%u,%u\n", gNdsStageMPCrossFloorLoopP0CrossHitCount, gNdsStageMPCrossFloorLoopP0FinalLineID, gNdsStageMPCrossFloorLoopP0TargetLineMatchCount, gNdsStageMPCrossFloorLoopP0FinalFloorOK',
            'printf "STAGE_MPCROSS_FLOOR_P1=%u,%d,%u\n", gNdsStageMPCrossFloorLoopP1CrossHitCount, gNdsStageMPCrossFloorLoopP1FinalLineID, gNdsStageMPCrossFloorLoopP1FinalFloorOK'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCrossFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPAdjustFloor) {
        $mpAdjustFloorCommands = @(
            'printf "STAGE_MPADJUST_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPAdjustFloorLoopResult, gNdsFighterMarioFoxStageMPAdjustFloorLoopSafeResult, gNdsFighterMarioFoxStageMPAdjustFloorLoopMask, gNdsFighterMarioFoxStageMPAdjustFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPAdjustFloorLoopCount',
            'printf "STAGE_MPADJUST_FLOOR_SETUP=%u,%u,%u,%u\n", gNdsStageMPAdjustFloorLoopPrepared, gNdsStageMPAdjustFloorLoopBaseMPCrossSeen, gNdsStageMPAdjustFloorLoopRunCallCount, gNdsStageMPAdjustFloorLoopUnsafeCount',
            'printf "STAGE_MPADJUST_FLOOR_CHECK=%u,%u,%u,%u,%u,%u\n", gNdsStageMPAdjustFloorLoopCheckLCallCount, gNdsStageMPAdjustFloorLoopCheckRCallCount, gNdsStageMPAdjustFloorLoopCheckLHitCount, gNdsStageMPAdjustFloorLoopCheckRHitCount, gNdsStageMPAdjustFloorLoopCheckLMissCount, gNdsStageMPAdjustFloorLoopCheckRMissCount',
            'printf "STAGE_MPADJUST_FLOOR_WALL=%u,%u,%u,%u,%u,%u\n", gNdsStageMPAdjustFloorLoopWallLCallCount, gNdsStageMPAdjustFloorLoopWallRCallCount, gNdsStageMPAdjustFloorLoopWallLHitCount, gNdsStageMPAdjustFloorLoopWallRHitCount, gNdsStageMPAdjustFloorLoopWallLMissCount, gNdsStageMPAdjustFloorLoopWallRMissCount',
            'printf "STAGE_MPADJUST_FLOOR_EDGE=%u,%u,%u,%u,%u,%u\n", gNdsStageMPAdjustFloorLoopEdgeUnderLCallCount, gNdsStageMPAdjustFloorLoopEdgeUnderRCallCount, gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount, gNdsStageMPAdjustFloorLoopAdjustLCallCount, gNdsStageMPAdjustFloorLoopAdjustRCallCount, gNdsStageMPAdjustFloorLoopNoAdjustCount',
            'printf "STAGE_MPADJUST_FLOOR_P0P1=%u,%u,%d,%d,%u,%u\n", gNdsStageMPAdjustFloorLoopP0RunCount, gNdsStageMPAdjustFloorLoopP1RunCount, gNdsStageMPAdjustFloorLoopP0FinalLineID, gNdsStageMPAdjustFloorLoopP1FinalLineID, gNdsStageMPAdjustFloorLoopP0FinalFloorOK, gNdsStageMPAdjustFloorLoopP1FinalFloorOK'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpAdjustFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPEdgeFloor) {
        $mpEdgeFloorCommands = @(
            'printf "STAGE_MPEDGE_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPEdgeFloorLoopResult, gNdsFighterMarioFoxStageMPEdgeFloorLoopSafeResult, gNdsFighterMarioFoxStageMPEdgeFloorLoopMask, gNdsFighterMarioFoxStageMPEdgeFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPEdgeFloorLoopCount',
            'printf "STAGE_MPEDGE_FLOOR_SETUP=%u,%u,%d,%u,%u\n", gNdsStageMPEdgeFloorLoopPrepared, gNdsStageMPEdgeFloorLoopBaseMPAdjustSeen, gNdsStageMPEdgeFloorLoopSelectedFloorLineID, gNdsStageMPEdgeFloorLoopSelectedFloorOK, gNdsStageMPEdgeFloorLoopUnsafeCount',
            'printf "STAGE_MPEDGE_FLOOR_EDGE=%u,%u,%u,%u,%u,%u\n", gNdsStageMPEdgeFloorLoopEdgeUnderLCallCount, gNdsStageMPEdgeFloorLoopEdgeUnderRCallCount, gNdsStageMPEdgeFloorLoopEdgeUnderLHitCount, gNdsStageMPEdgeFloorLoopEdgeUnderRHitCount, gNdsStageMPEdgeFloorLoopEdgeUnderLMissCount, gNdsStageMPEdgeFloorLoopEdgeUnderRMissCount',
            'printf "STAGE_MPEDGE_FLOOR_LINE=%d,%d,%u,%u,%u\n", gNdsStageMPEdgeFloorLoopEdgeUnderLLineID, gNdsStageMPEdgeFloorLoopEdgeUnderRLineID, gNdsStageMPEdgeFloorLoopEdgeUnderLLineKind, gNdsStageMPEdgeFloorLoopEdgeUnderRLineKind, gNdsStageMPAdjustFloorLoopEdgeUnderDeferredCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpEdgeFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPWallFloor) {
        $mpWallFloorCommands = @(
            'printf "STAGE_MPWALL_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPWallFloorLoopResult, gNdsFighterMarioFoxStageMPWallFloorLoopSafeResult, gNdsFighterMarioFoxStageMPWallFloorLoopMask, gNdsFighterMarioFoxStageMPWallFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPWallFloorLoopCount',
            'printf "STAGE_MPWALL_FLOOR_SETUP=%u,%u,%u,%u,%u,%u\n", gNdsStageMPWallFloorLoopPrepared, gNdsStageMPWallFloorLoopBaseMPEdgeSeen, gNdsStageMPWallFloorLoopProbeCount, gNdsStageMPWallFloorLoopProbeHitCount, gNdsStageMPWallFloorLoopProbeMissCount, gNdsStageMPWallFloorLoopUnsafeCount',
            'printf "STAGE_MPWALL_FLOOR_LINE=%d,%d,%u,%d,%u,%u,%u,%u\n", gNdsStageMPWallFloorLoopFloorLineID, gNdsStageMPWallFloorLoopWallLineID, gNdsStageMPWallFloorLoopWallLineKind, gNdsStageMPWallFloorLoopEdgeUnderLineID, gNdsStageMPWallFloorLoopSide, gNdsStageMPWallFloorLoopCheckHitCount, gNdsStageMPWallFloorLoopAdjustCallCount, gNdsStageMPWallFloorLoopFinalFloorOK',
            'printf "STAGE_MPWALL_FLOOR_POS=%d,%d,%d,%d,%d,%d\n", gNdsStageMPWallFloorLoopStartXMilli, gNdsStageMPWallFloorLoopStartYMilli, gNdsStageMPWallFloorLoopFinalXMilli, gNdsStageMPWallFloorLoopFinalYMilli, gNdsStageMPWallFloorLoopDeltaXMilli, gNdsStageMPWallFloorLoopDeltaYMilli',
            'printf "STAGE_MPWALL_HIT_SCOUT=%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPWallHitScoutRunCount, gNdsStageMPWallHitScoutFloorTestCount, gNdsStageMPWallHitScoutWallTestCount, gNdsStageMPWallHitScoutCandidateCount, gNdsStageMPWallHitScoutHitCount, gNdsStageMPWallHitScoutMissCount, gNdsStageMPWallHitScoutUnsafeCount',
            'printf "STAGE_MPWALL_HIT_SCOUT_LINE=%d,%d,%d,%u,%u,%u\n", gNdsStageMPWallHitScoutFloorLineID, gNdsStageMPWallHitScoutWallLineID, gNdsStageMPWallHitScoutEdgeUnderLineID, gNdsStageMPWallHitScoutSide, gNdsStageMPWallHitScoutWallLineKind, gNdsStageMPWallHitScoutFinalFloorOK',
            'printf "STAGE_MPWALL_HIT_SCOUT_POS=%d,%d,%d,%d,%d,%d\n", gNdsStageMPWallHitScoutStartXMilli, gNdsStageMPWallHitScoutStartYMilli, gNdsStageMPWallHitScoutFinalXMilli, gNdsStageMPWallHitScoutFinalYMilli, gNdsStageMPWallHitScoutDeltaXMilli, gNdsStageMPWallHitScoutDeltaYMilli',
            'printf "STAGE_MPWALL_HYRULE_SCOUT=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPWallHyruleScoutRelocResult, gNdsStageMPWallHyruleScoutGroundDataReady, gNdsStageMPWallHyruleScoutGeometryReady, gNdsStageMPWallHyruleScoutMapNodesReady, gNdsStageMPWallHyruleScoutRunCount, gNdsStageMPWallHyruleScoutFloorTestCount, gNdsStageMPWallHyruleScoutWallTestCount, gNdsStageMPWallHyruleScoutCandidateCount, gNdsStageMPWallHyruleScoutHitCount, gNdsStageMPWallHyruleScoutMissCount, gNdsStageMPWallHyruleScoutUnsafeCount',
            'printf "STAGE_MPWALL_HYRULE_SCOUT_LINE=%d,%d,%d,%u,%u,%u\n", gNdsStageMPWallHyruleScoutFloorLineID, gNdsStageMPWallHyruleScoutWallLineID, gNdsStageMPWallHyruleScoutEdgeUnderLineID, gNdsStageMPWallHyruleScoutSide, gNdsStageMPWallHyruleScoutWallLineKind, gNdsStageMPWallHyruleScoutFinalFloorOK',
            'printf "STAGE_MPWALL_HYRULE_SCOUT_POS=%d,%d,%d,%d,%d,%d\n", gNdsStageMPWallHyruleScoutStartXMilli, gNdsStageMPWallHyruleScoutStartYMilli, gNdsStageMPWallHyruleScoutFinalXMilli, gNdsStageMPWallHyruleScoutFinalYMilli, gNdsStageMPWallHyruleScoutDeltaXMilli, gNdsStageMPWallHyruleScoutDeltaYMilli'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpWallFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPWallHitFloor) {
        $mpWallHitFloorCommands = @(
            'printf "STAGE_MPWALLHIT_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPWallHitFloorLoopResult, gNdsFighterMarioFoxStageMPWallHitFloorLoopSafeResult, gNdsFighterMarioFoxStageMPWallHitFloorLoopMask, gNdsFighterMarioFoxStageMPWallHitFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPWallHitFloorLoopCount',
            'printf "STAGE_MPWALLHIT_FLOOR_SETUP=%u,%u,%#x,%u,%u,%u,%u\n", gNdsStageMPWallHitFloorLoopPrepared, gNdsStageMPWallHitFloorLoopBaseMPWallSeen, gNdsStageMPWallHitFloorLoopRelocResult, gNdsStageMPWallHitFloorLoopGroundDataReady, gNdsStageMPWallHitFloorLoopGeometryReady, gNdsStageMPWallHitFloorLoopMapNodesReady, gNdsStageMPWallHitFloorLoopUnsafeCount',
            'printf "STAGE_MPWALLHIT_FLOOR_COUNT=%u,%u,%u,%u,%u,%u\n", gNdsStageMPWallHitFloorLoopRunCount, gNdsStageMPWallHitFloorLoopFloorTestCount, gNdsStageMPWallHitFloorLoopWallTestCount, gNdsStageMPWallHitFloorLoopCandidateCount, gNdsStageMPWallHitFloorLoopHitCount, gNdsStageMPWallHitFloorLoopMissCount',
            'printf "STAGE_MPWALLHIT_FLOOR_LINE=%d,%d,%d,%u,%u,%u\n", gNdsStageMPWallHitFloorLoopFloorLineID, gNdsStageMPWallHitFloorLoopWallLineID, gNdsStageMPWallHitFloorLoopEdgeUnderLineID, gNdsStageMPWallHitFloorLoopSide, gNdsStageMPWallHitFloorLoopWallLineKind, gNdsStageMPWallHitFloorLoopFinalFloorOK',
            'printf "STAGE_MPWALLHIT_FLOOR_POS=%d,%d,%d,%d,%d,%d\n", gNdsStageMPWallHitFloorLoopStartXMilli, gNdsStageMPWallHitFloorLoopStartYMilli, gNdsStageMPWallHitFloorLoopFinalXMilli, gNdsStageMPWallHitFloorLoopFinalYMilli, gNdsStageMPWallHitFloorLoopDeltaXMilli, gNdsStageMPWallHitFloorLoopDeltaYMilli'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpWallHitFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPWallCopyFloor) {
        $mpWallCopyFloorCommands = @(
            'printf "STAGE_MPWALLCOPY_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPWallCopyFloorLoopResult, gNdsFighterMarioFoxStageMPWallCopyFloorLoopSafeResult, gNdsFighterMarioFoxStageMPWallCopyFloorLoopMask, gNdsFighterMarioFoxStageMPWallCopyFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPWallCopyFloorLoopCount',
            'printf "STAGE_MPWALLCOPY_BASE=%u,%u,%u,%u,%u,%u\n", gNdsStageMPWallCopyFloorLoopPrepared, gNdsStageMPWallCopyFloorLoopBaseMPWallHitSeen, gNdsStageMPWallCopyFloorLoopProcessAttachCount, gNdsStageMPWallCopyFloorLoopGObjProcessRunCount, gNdsStageMPWallCopyFloorLoopCallbackCount, gNdsStageMPWallCopyFloorLoopCopyBackCount',
            'printf "STAGE_MPWALLCOPY_SRC=%u,%u,%u,%u\n", gNdsStageMPWallCopyFloorLoopSourceFloorLineID, gNdsStageMPWallCopyFloorLoopSourceWallLineID, gNdsStageMPWallCopyFloorLoopSourceEdgeLineID, gNdsStageMPWallCopyFloorLoopSourceSide',
            'printf "STAGE_MPWALLCOPY_POS=%d,%d,%d,%d,%d,%d\n", gNdsStageMPWallCopyFloorLoopStartXMilli, gNdsStageMPWallCopyFloorLoopStartYMilli, gNdsStageMPWallCopyFloorLoopFinalXMilli, gNdsStageMPWallCopyFloorLoopFinalYMilli, gNdsStageMPWallCopyFloorLoopDeltaXMilli, gNdsStageMPWallCopyFloorLoopDeltaYMilli',
            'printf "STAGE_MPWALLCOPY_STATE=%u,%#x,%u,%u,%d,%d,%d,%d\n", gNdsStageMPWallCopyFloorLoopP0FinalFloorOK, gNdsStageMPWallCopyFloorLoopP0FinalMaskStat, gNdsStageMPWallCopyFloorLoopP0FinalGA, gNdsStageMPWallCopyFloorLoopUnsafeCount, gNdsStageMPWallCopyFloorLoopP1RootXBeforeMilli, gNdsStageMPWallCopyFloorLoopP1RootYBeforeMilli, gNdsStageMPWallCopyFloorLoopP1RootXAfterMilli, gNdsStageMPWallCopyFloorLoopP1RootYAfterMilli'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpWallCopyFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPPassFloor) {
        $mpPassFloorCommands = @(
            'printf "STAGE_MPPASS_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPPassFloorLoopResult, gNdsFighterMarioFoxStageMPPassFloorLoopSafeResult, gNdsFighterMarioFoxStageMPPassFloorLoopMask, gNdsFighterMarioFoxStageMPPassFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPPassFloorLoopCount',
            'printf "STAGE_MPPASS_BASE=%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassFloorLoopPrepared, gNdsStageMPPassFloorLoopBaseWallCopySeen, gNdsStageMPPassFloorLoopCandidateScanCount, gNdsStageMPPassFloorLoopCandidateCount, gNdsStageMPPassFloorLoopNoCandidateBlocker, gNdsStageMPPassFloorLoopUnsafeCount',
            'printf "STAGE_MPPASS_LINE=%d,%#x,%u\n", gNdsStageMPPassFloorLoopCandidateLineID, gNdsStageMPPassFloorLoopCandidateFlags, gNdsStageMPPassFloorLoopCandidateHasPass',
            'printf "STAGE_MPPASS_ROUTE=%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassFloorLoopRouteCount, gNdsStageMPPassFloorLoopSameLineRejectCount, gNdsStageMPPassFloorLoopDifferentLineAcceptCount, gNdsStageMPPassFloorLoopPassCallbackCount, gNdsStageMPPassFloorLoopPassCallbackAllowCount, gNdsStageMPPassFloorLoopPassCallbackDenyCount',
            'printf "STAGE_MPPASS_PROCESS=%u,%u,%u\n", gNdsStageMPPassFloorLoopProcessAttachCount, gNdsStageMPPassFloorLoopGObjProcessRunCount, gNdsStageMPPassFloorLoopCallbackCount',
            'printf "STAGE_MPPASS_P1=%d,%d,%d,%d,%u\n", gNdsStageMPPassFloorLoopP1RootXBeforeMilli, gNdsStageMPPassFloorLoopP1RootYBeforeMilli, gNdsStageMPPassFloorLoopP1RootXAfterMilli, gNdsStageMPPassFloorLoopP1RootYAfterMilli, gNdsStageMPPassFloorLoopP1RootUnchanged'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpPassFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPPlatformFloor) {
        $mpPlatformFloorCommands = @(
            'printf "STAGE_MPPLATFORM_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPPlatformFloorLoopResult, gNdsFighterMarioFoxStageMPPlatformFloorLoopSafeResult, gNdsFighterMarioFoxStageMPPlatformFloorLoopMask, gNdsFighterMarioFoxStageMPPlatformFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPPlatformFloorLoopCount',
            'printf "STAGE_MPPLATFORM_BASE=%u,%u,%u,%u\n", gNdsStageMPPlatformFloorLoopPrepared, gNdsStageMPPlatformFloorLoopBasePassSeen, gNdsStageMPPlatformFloorLoopProbeCount, gNdsStageMPPlatformFloorLoopUnsafeCount',
            'printf "STAGE_MPPLATFORM_LINE=%d,%#x,%u,%u,%u\n", gNdsStageMPPlatformFloorLoopLineID, gNdsStageMPPlatformFloorLoopLineFlags, gNdsStageMPPlatformFloorLoopLineHasPass, gNdsStageMPPlatformFloorLoopYakumonoID, gNdsStageMPPlatformFloorLoopYakumonoCount',
            'printf "STAGE_MPPLATFORM_DOBJ=%u,%u,%u,%u,%#x\n", gNdsStageMPPlatformFloorLoopDObjPresent, gNdsStageMPPlatformFloorLoopDObjStatus, gNdsStageMPPlatformFloorLoopDObjAnimPresent, gNdsStageMPPlatformFloorLoopPredicateResult, gNdsStageMPPlatformFloorLoopBlockerMask'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpPlatformFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPPlatformTickFloor) {
        $mpPlatformTickFloorCommands = @(
            'printf "STAGE_MPPLATFORM_TICK=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPPlatformTickFloorLoopResult, gNdsFighterMarioFoxStageMPPlatformTickFloorLoopSafeResult, gNdsFighterMarioFoxStageMPPlatformTickFloorLoopMask, gNdsFighterMarioFoxStageMPPlatformTickFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPPlatformTickFloorLoopCount',
            'printf "STAGE_MPPLATFORM_TICK_STEP=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPlatformTickFloorLoopPrepared, gNdsStageMPPlatformTickFloorLoopBaseActiveSeen, gNdsStageMPPlatformTickFloorLoopSetOnCount, gNdsStageMPPlatformTickFloorLoopAdvanceCount, gNdsStageMPPlatformTickFloorLoopUnsafeCount, gNdsStageMPPlatformTickFloorLoopUpdateTicBefore, gNdsStageMPPlatformTickFloorLoopUpdateTicAfter, gNdsStageMPPlatformTickFloorLoopDObjPresent, gNdsStageMPPlatformTickFloorLoopStatusBefore, gNdsStageMPPlatformTickFloorLoopStatusAfter, gNdsStageMPPlatformTickFloorLoopPredicateAfter'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpPlatformTickFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPPassInputLoop) {
        $mpPassInputLoopCommands = @(
            'printf "STAGE_MPPASS_INPUT=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPPassInputLoopResult, gNdsFighterMarioFoxStageMPPassInputLoopSafeResult, gNdsFighterMarioFoxStageMPPassInputLoopMask, gNdsFighterMarioFoxStageMPPassInputLoopDeferredMask, gNdsFighterMarioFoxStageMPPassInputLoopCount',
            'printf "STAGE_MPPASS_INPUT_SETUP=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassInputLoopPrepared, gNdsStageMPPassInputLoopBasePlatformTickSeen, gNdsStageMPPassInputLoopCheckCallCount, gNdsStageMPPassInputLoopCheckSuccessCount, gNdsStageMPPassInputLoopSquatSetCount, gNdsStageMPPassInputLoopSquatProcCount, gNdsStageMPPassInputLoopGotoPassCount, gNdsStageMPPassInputLoopPassSetCount, gNdsStageMPPassInputLoopSetAirCount, gNdsStageMPPassInputLoopClampCount, gNdsStageMPPassInputLoopUnsafeCount',
            'printf "STAGE_MPPASS_INPUT_STATE=%d,%#x,%d,%u,%u,%u,%u,%u,%u,%d,%d,%d\n", gNdsStageMPPassInputLoopLineID, gNdsStageMPPassInputLoopFlags, gNdsStageMPPassInputLoopStickY, gNdsStageMPPassInputLoopTapYBefore, gNdsStageMPPassInputLoopTapYAfter, gNdsStageMPPassInputLoopStatusBefore, gNdsStageMPPassInputLoopStatusAfterSquat, gNdsStageMPPassInputLoopStatusAfterPass, gNdsStageMPPassInputLoopGAAfterPass, gNdsStageMPPassInputLoopIgnoreLineID, gNdsStageMPPassInputLoopPassWaitInitial, gNdsStageMPPassInputLoopPassWaitFinal',
            'printf "STAGE_MPPASS_INPUT_SQUATRV=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassInputLoopSquatWaitSetCount, gNdsStageMPPassInputLoopSquatWaitProcCount, gNdsStageMPPassInputLoopSquatRvSetCount, gNdsStageMPPassInputLoopSquatRvEndCount, gNdsStageMPPassInputLoopStatusAfterSquatWait, gNdsStageMPPassInputLoopStatusAfterSquatRv, gNdsStageMPPassInputLoopStatusAfterSquatRvEnd, gNdsStageMPPassInputLoopSquatRvCallbackMask, gNdsStageMPPassInputLoopGAAfterSquatRvEnd'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpPassInputLoopCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPPlatformPosFloor) {
        $mpPlatformPosFloorCommands = @(
            'printf "STAGE_MPPLATFORM_POS=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPPlatformPosFloorLoopResult, gNdsFighterMarioFoxStageMPPlatformPosFloorLoopSafeResult, gNdsFighterMarioFoxStageMPPlatformPosFloorLoopMask, gNdsFighterMarioFoxStageMPPlatformPosFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPPlatformPosFloorLoopCount',
            'printf "STAGE_MPPLATFORM_POS_SETUP=%u,%u,%u,%u,%d,%u,%u,%u,%u,%u\n", gNdsStageMPPlatformPosFloorLoopPrepared, gNdsStageMPPlatformPosFloorLoopBasePassInputSeen, gNdsStageMPPlatformPosFloorLoopSetPosCount, gNdsStageMPPlatformPosFloorLoopUnsafeCount, gNdsStageMPPlatformPosFloorLoopLineID, gNdsStageMPPlatformPosFloorLoopYakumonoID, gNdsStageMPPlatformPosFloorLoopDObjPresent, gNdsStageMPPlatformPosFloorLoopStatusBefore, gNdsStageMPPlatformPosFloorLoopStatusAfter, gNdsStageMPPlatformPosFloorLoopPredicateAfter',
            'printf "STAGE_MPPLATFORM_POS_VEC=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsStageMPPlatformPosFloorLoopBeforeXMilli, gNdsStageMPPlatformPosFloorLoopBeforeYMilli, gNdsStageMPPlatformPosFloorLoopBeforeZMilli, gNdsStageMPPlatformPosFloorLoopTargetXMilli, gNdsStageMPPlatformPosFloorLoopTargetYMilli, gNdsStageMPPlatformPosFloorLoopTargetZMilli, gNdsStageMPPlatformPosFloorLoopAfterXMilli, gNdsStageMPPlatformPosFloorLoopAfterYMilli, gNdsStageMPPlatformPosFloorLoopAfterZMilli, gNdsStageMPPlatformPosFloorLoopSpeedXMilli, gNdsStageMPPlatformPosFloorLoopSpeedYMilli, gNdsStageMPPlatformPosFloorLoopSpeedZMilli'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpPlatformPosFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPPlatformSpeedFloor) {
        $mpPlatformSpeedFloorCommands = @(
            'printf "STAGE_MPPLATFORM_SPEED=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopResult, gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopSafeResult, gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopMask, gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPPlatformSpeedFloorLoopCount',
            'printf "STAGE_MPPLATFORM_SPEED_SETUP=%u,%u,%u,%u,%d,%u,%u,%u\n", gNdsStageMPPlatformSpeedFloorLoopPrepared, gNdsStageMPPlatformSpeedFloorLoopBasePosSeen, gNdsStageMPPlatformSpeedFloorLoopGetSpeedCount, gNdsStageMPPlatformSpeedFloorLoopUnsafeCount, gNdsStageMPPlatformSpeedFloorLoopLineID, gNdsStageMPPlatformSpeedFloorLoopYakumonoID, gNdsStageMPPlatformSpeedFloorLoopStatus, gNdsStageMPPlatformSpeedFloorLoopPredicate',
            'printf "STAGE_MPPLATFORM_SPEED_VEC=%d,%d,%d,%d,%d,%d\n", gNdsStageMPPlatformSpeedFloorLoopExpectedXMilli, gNdsStageMPPlatformSpeedFloorLoopExpectedYMilli, gNdsStageMPPlatformSpeedFloorLoopExpectedZMilli, gNdsStageMPPlatformSpeedFloorLoopReadXMilli, gNdsStageMPPlatformSpeedFloorLoopReadYMilli, gNdsStageMPPlatformSpeedFloorLoopReadZMilli',
            'printf "STAGE_MPPLATFORM_SPEED_DYNAMIC=%u,%u,%u,%d,%u,%d,%d,%d,%d\n", gNdsStageMPPlatformSpeedFloorLoopDynamicBranchCount, gNdsStageMPPlatformSpeedFloorLoopDynamicProbeCount, gNdsStageMPPlatformSpeedFloorLoopDynamicHitCount, gNdsStageMPPlatformSpeedFloorLoopDynamicLineID, gNdsStageMPPlatformSpeedFloorLoopDynamicYakumonoID, gNdsStageMPPlatformSpeedFloorLoopDynamicSpeedXMilli, gNdsStageMPPlatformSpeedFloorLoopDynamicSpeedYMilli, gNdsStageMPPlatformSpeedFloorLoopDynamicGaXMilli, gNdsStageMPPlatformSpeedFloorLoopDynamicGaYMilli',
            'printf "STAGE_MPPLATFORM_SPEED_DYNAMIC_CEIL=%u,%u,%d,%u,%d,%d\n", gNdsStageMPPlatformSpeedFloorLoopDynamicCeilProbeCount, gNdsStageMPPlatformSpeedFloorLoopDynamicCeilHitCount, gNdsStageMPPlatformSpeedFloorLoopDynamicCeilLineID, gNdsStageMPPlatformSpeedFloorLoopDynamicCeilYakumonoID, gNdsStageMPPlatformSpeedFloorLoopDynamicCeilGaXMilli, gNdsStageMPPlatformSpeedFloorLoopDynamicCeilGaYMilli',
            'printf "STAGE_MPPLATFORM_SPEED_DYNAMIC_WALL=%u,%u,%d,%u,%u,%d,%d\n", gNdsStageMPPlatformSpeedFloorLoopDynamicWallProbeCount, gNdsStageMPPlatformSpeedFloorLoopDynamicWallHitCount, gNdsStageMPPlatformSpeedFloorLoopDynamicWallLineID, gNdsStageMPPlatformSpeedFloorLoopDynamicWallYakumonoID, gNdsStageMPPlatformSpeedFloorLoopDynamicWallKind, gNdsStageMPPlatformSpeedFloorLoopDynamicWallGaXMilli, gNdsStageMPPlatformSpeedFloorLoopDynamicWallGaYMilli',
            'printf "STAGE_MPPLATFORM_SPEED_PROCESS_WALL=%u,%u,%d,%u,%#x,%u\n", gNdsStageMPPlatformSpeedFloorLoopProcessWallProbeCount, gNdsStageMPPlatformSpeedFloorLoopProcessWallHitCount, gNdsStageMPPlatformSpeedFloorLoopProcessWallLineID, gNdsStageMPPlatformSpeedFloorLoopProcessWallKind, gNdsStageMPPlatformSpeedFloorLoopProcessWallMaskCurr, gNdsStageMPPlatformSpeedFloorLoopProcessWallMultiCount',
            'printf "STAGE_MPPLATFORM_SPEED_ANIM=%u,%u,%u,%u,%u,%d,%d,%d\n", gNdsStageMPPlatformSpeedFloorLoopAnimPlayCount, gNdsStageMPPlatformSpeedFloorLoopAnimTicBefore, gNdsStageMPPlatformSpeedFloorLoopAnimTicAfter, gNdsStageMPPlatformSpeedFloorLoopAnimStatusBefore, gNdsStageMPPlatformSpeedFloorLoopAnimStatusAfter, gNdsStageMPPlatformSpeedFloorLoopAnimSpeedXMilli, gNdsStageMPPlatformSpeedFloorLoopAnimSpeedYMilli, gNdsStageMPPlatformSpeedFloorLoopAnimSpeedZMilli',
            'printf "STAGE_MPPLATFORM_SPEED_BOUNDS=%u,%d,%d,%d,%d\n", gNdsStageMPPlatformSpeedFloorLoopBoundsUpdateCount, gNdsStageMPPlatformSpeedFloorLoopBoundsDiffTopMilli, gNdsStageMPPlatformSpeedFloorLoopBoundsDiffBottomMilli, gNdsStageMPPlatformSpeedFloorLoopBoundsDiffRightMilli, gNdsStageMPPlatformSpeedFloorLoopBoundsDiffLeftMilli',
            'printf "STAGE_MPPLATFORM_SPEED_STAGE_ANIM=%#x,%u,%u,%u\n", gNdsStageMPPlatformSpeedFloorLoopStageAnimMask, gNdsStageMPPlatformSpeedFloorLoopStageAnimDObjCount, gNdsStageMPPlatformSpeedFloorLoopStageAnimMObjCount, gNdsStageMPPlatformSpeedFloorLoopStageAnimCallbackCount',
            'printf "STAGE_INISHIE_ASSET=%#x,%#x,%u,%u,%u,%u,%u\n", gNdsStageInishieRelocResult, gNdsStageInishieMapHeaderOffset, gNdsStageInishieGroundDataPtrReady, gNdsStageInishieGeometryPtrReady, gNdsStageInishieMapNodesPtrReady, gNdsStageInishieYakumonoCount, gNdsStageInishieMapObjCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpPlatformSpeedFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageInishieScaleLoop) {
        $stageInishieScaleLoopCommands = @(
            'printf "STAGE_INISHIE_SCALE=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageInishieScaleLoopResult, gNdsFighterMarioFoxStageInishieScaleLoopSafeResult, gNdsFighterMarioFoxStageInishieScaleLoopMask, gNdsFighterMarioFoxStageInishieScaleLoopDeferredMask, gNdsFighterMarioFoxStageInishieScaleLoopCount',
            'printf "STAGE_INISHIE_SCALE_SETUP=%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%#x,%#x\n", gNdsStageInishieScaleLoopPrepared, gNdsStageInishieScaleLoopBaseSpeedSeen, gNdsStageInishieScaleLoopShellMakeCount, gNdsStageInishieScaleLoopUpdateCount, gNdsStageInishieScaleLoopSetPosCount, gNdsStageInishieScaleLoopSetOnCount, gNdsStageInishieScaleLoopUnsafeCount, gNdsStageInishieScaleLoopDObjMask, gNdsStageInishieScaleLoopLineMask, gNdsStageInishieScaleLoopSetPosMask, gNdsStageInishieScaleLoopSetOnMask',
            'printf "STAGE_INISHIE_SCALE_SOURCE=%u,%u,%u,%#x\n", gNdsStageInishieScaleLoopSourceSetupStep, gNdsStageInishieScaleLoopSourceSetupGObjCountBefore, gNdsStageInishieScaleLoopSourceSetupDObjCountBefore, gNdsStageInishieScaleLoopSourceSetupGObjReadyMask',
            'printf "STAGE_INISHIE_SCALE_DISPLAY=%#x,%u,%u,%u,%u\n", gNdsStageInishieScaleLoopSourceDisplayMask, gNdsStageInishieScaleLoopSourceDisplayCount, gNdsStageInishieScaleLoopSourceDisplayCommands, gNdsStageInishieScaleLoopSourceDisplayTriangles, gNdsStageInishieScaleLoopSourceDisplayBlocker',
            'printf "STAGE_INISHIE_SCALE_MATERIAL=%#x,%u,%#x,%#x\n", gNdsStageInishieScaleLoopSourceTextureMask, gNdsStageInishieScaleLoopSourceTextureCommands, gNdsStageInishieScaleLoopSourceTextureImage, gNdsStageInishieScaleLoopSourceTextureSize',
            'printf "STAGE_INISHIE_SCALE_PREVIEW=%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageInishieScaleLoopSourcePreviewMask, gNdsStageInishieScaleLoopSourcePreviewDObjCount, gNdsStageInishieScaleLoopSourcePreviewVertexCount, gNdsStageInishieScaleLoopSourcePreviewTriangleCount, gNdsStageInishieScaleLoopSourcePreviewValidTriangleCount, gNdsStageInishieScaleLoopSourcePreviewPixelCount, gNdsStageInishieScaleLoopSourcePreviewCommitDelta, gNdsStageInishieScaleLoopSourcePreviewBlocker',
            'printf "STAGE_INISHIE_SCALE_LINES=%d,%d,%u,%u,%u,%u\n", gNdsStageInishieScaleLoopLeftLineID, gNdsStageInishieScaleLoopRightLineID, gNdsStageInishieScaleLoopLeftMapObjKind, gNdsStageInishieScaleLoopRightMapObjKind, gNdsStageInishieScaleLoopStatusBefore, gNdsStageInishieScaleLoopStatusAfter',
            'printf "STAGE_INISHIE_SCALE_ALT=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsStageInishieScaleLoopAltBeforeMilli, gNdsStageInishieScaleLoopAltAfterMilli, gNdsStageInishieScaleLoopLeftBaseYMilli, gNdsStageInishieScaleLoopRightBaseYMilli, gNdsStageInishieScaleLoopLeftYBeforeMilli, gNdsStageInishieScaleLoopRightYBeforeMilli, gNdsStageInishieScaleLoopLeftYAfterMilli, gNdsStageInishieScaleLoopRightYAfterMilli, gNdsStageInishieScaleLoopLeftSpeedYMilli, gNdsStageInishieScaleLoopRightSpeedYMilli',
            'printf "STAGE_INISHIE_SCALE_FALL=%u,%u,%u,%u,%d,%d,%d,%d,%d,%d\n", gNdsStageInishieScaleLoopFallSetPosCount, gNdsStageInishieScaleLoopFallSparkleCount, gNdsStageInishieScaleLoopFallStatusAfterWait, gNdsStageInishieScaleLoopFallStatusAfterFall, gNdsStageInishieScaleLoopFallAltAfterWaitMilli, gNdsStageInishieScaleLoopFallAccelAfterFallMilli, gNdsStageInishieScaleLoopFallLeftYAfterFallMilli, gNdsStageInishieScaleLoopFallRightYAfterFallMilli, gNdsStageInishieScaleLoopFallLeftSpeedYMilli, gNdsStageInishieScaleLoopFallRightSpeedYMilli',
            'printf "STAGE_INISHIE_SCALE_STEP=%u,%#x,%u,%#x,%u,%u,%u,%u,%d,%d,%d,%d,%d\n", gNdsStageInishieScaleLoopStepSetPosCount, gNdsStageInishieScaleLoopStepSetPosMask, gNdsStageInishieScaleLoopStepSetOnCount, gNdsStageInishieScaleLoopStepSetOnMask, gNdsStageInishieScaleLoopStepWaitBefore, gNdsStageInishieScaleLoopStepWaitAfter, gNdsStageInishieScaleLoopStepStatusAfterSleep, gNdsStageInishieScaleLoopStepStatusAfterRetract, gNdsStageInishieScaleLoopStepAltAfterRetractMilli, gNdsStageInishieScaleLoopStepLeftYAfterRetractMilli, gNdsStageInishieScaleLoopStepRightYAfterRetractMilli, gNdsStageInishieScaleLoopStepLeftSpeedYMilli, gNdsStageInishieScaleLoopStepRightSpeedYMilli'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $stageInishieScaleLoopCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPStaleFloor) {
        $mpStaleFloorCommands = @(
            'printf "STAGE_MPSTALE_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPStaleFloorLoopResult, gNdsFighterMarioFoxStageMPStaleFloorLoopSafeResult, gNdsFighterMarioFoxStageMPStaleFloorLoopMask, gNdsFighterMarioFoxStageMPStaleFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPStaleFloorLoopCount',
            'printf "STAGE_MPSTALE_FLOOR_SETUP=%u,%u,%u,%u,%u,%d,%d,%d,%d,%u\n", gNdsStageMPStaleFloorLoopPrepared, gNdsStageMPStaleFloorLoopBaseMPWallSeen, gNdsStageMPStaleFloorLoopPrimeAttemptCount, gNdsStageMPStaleFloorLoopPrimeHitCount, gNdsStageMPStaleFloorLoopPrimeMissCount, gNdsStageMPStaleFloorLoopStaleLineID, gNdsStageMPStaleFloorLoopTargetLineID, gNdsStageMPStaleFloorLoopTargetXMilli, gNdsStageMPStaleFloorLoopTargetYMilli, gNdsStageMPStaleFloorLoopUnsafeCount',
            'printf "STAGE_MPSTALE_FLOOR_LIVE=%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPStaleFloorLoopLiveSecondFloorCallCount, gNdsStageMPStaleFloorLoopLiveSecondFloorHitCount, gNdsStageMPStaleFloorLoopLiveSecondFloorMissCount, gNdsStageMPStaleFloorLoopLiveAcceptedNewLineCount, gNdsStageMPStaleFloorLoopLiveLandingFloorCount, gNdsStageMPStaleFloorLoopLiveFloorEdgeAdjustCount, gNdsStageMPStaleFloorLoopLiveCollEndClearCount',
            'printf "STAGE_MPSTALE_FLOOR_P0P1=%d,%d,%u,%u,%u\n", gNdsStageMPStaleFloorLoopP0FinalLineID, gNdsStageMPStaleFloorLoopP1FinalLineID, gNdsStageMPStaleFloorLoopP0TargetLineMatchCount, gNdsStageMPStaleFloorLoopP0FinalFloorOK, gNdsStageMPStaleFloorLoopP1FinalFloorOK'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpStaleFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPLiveStaleFloor) {
        $mpLiveStaleFloorCommands = @(
            'printf "STAGE_MPLIVESTALE_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPLiveStaleFloorLoopResult, gNdsFighterMarioFoxStageMPLiveStaleFloorLoopSafeResult, gNdsFighterMarioFoxStageMPLiveStaleFloorLoopMask, gNdsFighterMarioFoxStageMPLiveStaleFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPLiveStaleFloorLoopCount',
            'printf "STAGE_MPLIVESTALE_FLOOR_SETUP=%u,%u,%u,%u,%u,%d,%d,%d,%d,%u,%u\n", gNdsStageMPLiveStaleFloorLoopPrepared, gNdsStageMPLiveStaleFloorLoopBaseMPStaleSeen, gNdsStageMPLiveStaleFloorLoopPrimeAttemptCount, gNdsStageMPLiveStaleFloorLoopPrimeHitCount, gNdsStageMPLiveStaleFloorLoopPrimeMissCount, gNdsStageMPLiveStaleFloorLoopStaleLineID, gNdsStageMPLiveStaleFloorLoopTargetLineID, gNdsStageMPLiveStaleFloorLoopTargetXMilli, gNdsStageMPLiveStaleFloorLoopTargetYMilli, gNdsStageMPLiveStaleFloorLoopSelectedCallbackCount, gNdsStageMPLiveStaleFloorLoopUnsafeCount',
            'printf "STAGE_MPLIVESTALE_FLOOR_LIVE=%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPLiveStaleFloorLoopLiveSecondFloorCallCount, gNdsStageMPLiveStaleFloorLoopLiveSecondFloorHitCount, gNdsStageMPLiveStaleFloorLoopLiveSecondFloorMissCount, gNdsStageMPLiveStaleFloorLoopLiveAcceptedNewLineCount, gNdsStageMPLiveStaleFloorLoopLiveLandingFloorCount, gNdsStageMPLiveStaleFloorLoopLiveFloorEdgeAdjustCount, gNdsStageMPLiveStaleFloorLoopLiveCollEndClearCount',
            'printf "STAGE_MPLIVESTALE_FLOOR_P0P1=%d,%d,%u,%u,%u\n", gNdsStageMPLiveStaleFloorLoopP0FinalLineID, gNdsStageMPLiveStaleFloorLoopP1FinalLineID, gNdsStageMPLiveStaleFloorLoopP0TargetLineMatchCount, gNdsStageMPLiveStaleFloorLoopP0FinalFloorOK, gNdsStageMPLiveStaleFloorLoopP1FinalFloorOK'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpLiveStaleFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPMotionStaleFloor) {
        $mpMotionStaleFloorCommands = @(
            'printf "STAGE_MPMOTIONSTALE_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPMotionStaleFloorLoopResult, gNdsFighterMarioFoxStageMPMotionStaleFloorLoopSafeResult, gNdsFighterMarioFoxStageMPMotionStaleFloorLoopMask, gNdsFighterMarioFoxStageMPMotionStaleFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPMotionStaleFloorLoopCount',
            'printf "STAGE_MPMOTIONSTALE_FLOOR_SETUP=%u,%u,%u,%u,%u,%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsStageMPMotionStaleFloorLoopPrepared, gNdsStageMPMotionStaleFloorLoopBaseMPLiveStaleSeen, gNdsStageMPMotionStaleFloorLoopPrimeAttemptCount, gNdsStageMPMotionStaleFloorLoopPrimeHitCount, gNdsStageMPMotionStaleFloorLoopPrimeMissCount, gNdsStageMPMotionStaleFloorLoopStaleLineID, gNdsStageMPMotionStaleFloorLoopTargetLineID, gNdsStageMPMotionStaleFloorLoopTargetXMilli, gNdsStageMPMotionStaleFloorLoopTargetYMilli, gNdsStageMPMotionStaleFloorLoopMutationCount, gNdsStageMPMotionStaleFloorLoopUpdateHitCount, gNdsStageMPMotionStaleFloorLoopTargetMatchCount, gNdsStageMPMotionStaleFloorLoopUnsafeCount',
            'printf "STAGE_MPMOTIONSTALE_FLOOR_POS=%d,%d,%d,%d,%d,%d\n", gNdsStageMPMotionStaleFloorLoopP0PrevXMilli, gNdsStageMPMotionStaleFloorLoopP0PrevYMilli, gNdsStageMPMotionStaleFloorLoopP0TargetXMilli, gNdsStageMPMotionStaleFloorLoopP0TargetYMilli, gNdsStageMPMotionStaleFloorLoopP0FinalXMilli, gNdsStageMPMotionStaleFloorLoopP0FinalYMilli',
            'printf "STAGE_MPMOTIONSTALE_FLOOR_P0P1=%d,%d,%u,%u\n", gNdsStageMPMotionStaleFloorLoopP0FinalLineID, gNdsStageMPMotionStaleFloorLoopP1FinalLineID, gNdsStageMPMotionStaleFloorLoopP0FinalFloorOK, gNdsStageMPMotionStaleFloorLoopP1FinalFloorOK'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpMotionStaleFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffStatusFloor) {
        $mpCliffStatusFloorCommands = @(
            'printf "STAGE_MPCLIFFSTATUS_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffStatusFloorLoopResult, gNdsFighterMarioFoxStageMPCliffStatusFloorLoopSafeResult, gNdsFighterMarioFoxStageMPCliffStatusFloorLoopMask, gNdsFighterMarioFoxStageMPCliffStatusFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffStatusFloorLoopCount',
            'printf "STAGE_MPCLIFFSTATUS_FLOOR_SETUP=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffStatusFloorLoopPrepared, gNdsStageMPCliffStatusFloorLoopBaseMPMotionStaleSeen, gNdsStageMPCliffStatusFloorLoopProcCallCount, gNdsStageMPCliffStatusFloorLoopCheckFalseCount, gNdsStageMPCliffStatusFloorLoopCheckTrueCount, gNdsStageMPCliffStatusFloorLoopFloorEdgeBranchCount, gNdsStageMPCliffStatusFloorLoopFallBranchCount, gNdsStageMPCliffStatusFloorLoopUnsafeCount',
            'printf "STAGE_MPCLIFFSTATUS_FLOOR_STATUS=%u,%u,%u,%u\n", gNdsStageMPCliffStatusFloorLoopOttottoSetStatusCallCount, gNdsStageMPCliffStatusFloorLoopFallSetStatusCallCount, gNdsStageMPCliffStatusFloorLoopStatusSetCount, gNdsStageMPCliffStatusFloorLoopAirSetCount',
            'printf "STAGE_MPCLIFFSTATUS_FLOOR_P0P1=%u,%u,%u,%d,%u,%u,%u,%u,%d,%u\n", gNdsStageMPCliffStatusFloorLoopP0StatusFinal, gNdsStageMPCliffStatusFloorLoopP0MotionFinal, gNdsStageMPCliffStatusFloorLoopP0GAFinal, gNdsStageMPCliffStatusFloorLoopP0LineFinal, gNdsStageMPCliffStatusFloorLoopP0FloorEdgeMask, gNdsStageMPCliffStatusFloorLoopP1StatusFinal, gNdsStageMPCliffStatusFloorLoopP1MotionFinal, gNdsStageMPCliffStatusFloorLoopP1GAFinal, gNdsStageMPCliffStatusFloorLoopP1LineFinal, gNdsStageMPCliffStatusFloorLoopP1FloorEdgeMask'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffStatusFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffTickFloor) {
        $mpCliffTickFloorCommands = @(
            'printf "STAGE_MPCLIFFTICK_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffTickFloorLoopResult, gNdsFighterMarioFoxStageMPCliffTickFloorLoopSafeResult, gNdsFighterMarioFoxStageMPCliffTickFloorLoopMask, gNdsFighterMarioFoxStageMPCliffTickFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffTickFloorLoopCount',
            'printf "STAGE_MPCLIFFTICK_FLOOR_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffTickFloorLoopPrepared, gNdsStageMPCliffTickFloorLoopBaseMPCliffStatusSeen, gNdsStageMPCliffTickFloorLoopOttottoUpdateCallCount, gNdsStageMPCliffTickFloorLoopOttottoInterruptCallCount, gNdsStageMPCliffTickFloorLoopOttottoMapCallCount, gNdsStageMPCliffTickFloorLoopOttottoAnimEndCheckCount, gNdsStageMPCliffTickFloorLoopOttottoFloorCheckCount, gNdsStageMPCliffTickFloorLoopOttottoFloorHitCount, gNdsStageMPCliffTickFloorLoopFallInterruptCallCount, gNdsStageMPCliffTickFloorLoopFallSpecialAirCheckCount, gNdsStageMPCliffTickFloorLoopFallAttackAirCheckCount, gNdsStageMPCliffTickFloorLoopFallJumpAerialCheckCount',
            'printf "STAGE_MPCLIFFTICK_FLOOR_STATUS=%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u\n", gNdsStageMPCliffTickFloorLoopP0StatusBefore, gNdsStageMPCliffTickFloorLoopP0MotionBefore, gNdsStageMPCliffTickFloorLoopP0GABefore, gNdsStageMPCliffTickFloorLoopP0StatusAfter, gNdsStageMPCliffTickFloorLoopP0MotionAfter, gNdsStageMPCliffTickFloorLoopP0GAAfter, gNdsStageMPCliffTickFloorLoopP0LineAfter, gNdsStageMPCliffTickFloorLoopP1StatusBefore, gNdsStageMPCliffTickFloorLoopP1MotionBefore, gNdsStageMPCliffTickFloorLoopP1GABefore, gNdsStageMPCliffTickFloorLoopP1StatusAfter, gNdsStageMPCliffTickFloorLoopP1MotionAfter, gNdsStageMPCliffTickFloorLoopP1GAAfter, gNdsStageMPCliffTickFloorLoopP1LineAfter, gNdsStageMPCliffTickFloorLoopStatusSetCount, gNdsStageMPCliffTickFloorLoopOttottoWaitSetStatusCallCount, gNdsStageMPCliffTickFloorLoopUnsafeCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffTickFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPFallMapFloor) {
        $mpFallMapFloorCommands = @(
            'printf "STAGE_MPFALLMAP_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPFallMapFloorLoopResult, gNdsFighterMarioFoxStageMPFallMapFloorLoopSafeResult, gNdsFighterMarioFoxStageMPFallMapFloorLoopMask, gNdsFighterMarioFoxStageMPFallMapFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPFallMapFloorLoopCount',
            'printf "STAGE_MPFALLMAP_FLOOR_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPFallMapFloorLoopPrepared, gNdsStageMPFallMapFloorLoopBaseMPCliffTickSeen, gNdsStageMPFallMapFloorLoopPhysicsCallbackCount, gNdsStageMPFallMapFloorLoopFastFallCheckCount, gNdsStageMPFallMapFloorLoopGravityCallCount, gNdsStageMPFallMapFloorLoopAirDriftCallCount, gNdsStageMPFallMapFloorLoopAirFrictionCallCount, gNdsStageMPFallMapFloorLoopIntegrateCount, gNdsStageMPFallMapFloorLoopMapCallbackCount, gNdsStageMPFallMapFloorLoopMapNoCollisionCount',
            'printf "STAGE_MPFALLMAP_FLOOR_STATUS=%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%u\n", gNdsStageMPFallMapFloorLoopP1StatusBefore, gNdsStageMPFallMapFloorLoopP1MotionBefore, gNdsStageMPFallMapFloorLoopP1GABefore, gNdsStageMPFallMapFloorLoopP1StatusAfter, gNdsStageMPFallMapFloorLoopP1MotionAfter, gNdsStageMPFallMapFloorLoopP1GAAfter, gNdsStageMPFallMapFloorLoopP1LineBefore, gNdsStageMPFallMapFloorLoopP1LineAfter, gNdsStageMPFallMapFloorLoopP1RootYBeforeMilli, gNdsStageMPFallMapFloorLoopP1RootYAfterMilli, gNdsStageMPFallMapFloorLoopP1VelYBeforeMilli, gNdsStageMPFallMapFloorLoopP1VelYAfterMilli, gNdsStageMPFallMapFloorLoopUnsafeCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpFallMapFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPFallLandFloor) {
        $mpFallLandFloorCommands = @(
            'printf "STAGE_MPFALLLAND_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPFallLandFloorLoopResult, gNdsFighterMarioFoxStageMPFallLandFloorLoopSafeResult, gNdsFighterMarioFoxStageMPFallLandFloorLoopMask, gNdsFighterMarioFoxStageMPFallLandFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPFallLandFloorLoopCount',
            'printf "STAGE_MPFALLLAND_FLOOR_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPFallLandFloorLoopPrepared, gNdsStageMPFallLandFloorLoopBaseMPFallMapSeen, gNdsStageMPFallLandFloorLoopPhysicsCallbackCount, gNdsStageMPFallLandFloorLoopFastFallCheckCount, gNdsStageMPFallLandFloorLoopGravityCallCount, gNdsStageMPFallLandFloorLoopAirDriftCallCount, gNdsStageMPFallLandFloorLoopAirFrictionCallCount, gNdsStageMPFallLandFloorLoopIntegrateCount, gNdsStageMPFallLandFloorLoopMapCallbackCount, gNdsStageMPFallLandFloorLoopMapFloorCollisionCount, gNdsStageMPFallLandFloorLoopSetLandingFloorCount, gNdsStageMPFallLandFloorLoopWaitOrLandingCount, gNdsStageMPFallLandFloorLoopLandingSetStatusCallCount, gNdsStageMPFallLandFloorLoopLandingParamCallCount, gNdsStageMPFallLandFloorLoopStatusSetCallCount',
            'printf "STAGE_MPFALLLAND_FLOOR_STATUS=%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u\n", gNdsStageMPFallLandFloorLoopP1StatusBefore, gNdsStageMPFallLandFloorLoopP1MotionBefore, gNdsStageMPFallLandFloorLoopP1GABefore, gNdsStageMPFallLandFloorLoopP1StatusAfter, gNdsStageMPFallLandFloorLoopP1MotionAfter, gNdsStageMPFallLandFloorLoopP1GAAfter, gNdsStageMPFallLandFloorLoopP1LineBefore, gNdsStageMPFallLandFloorLoopP1LineAfter, gNdsStageMPFallLandFloorLoopP1FloorYMilli, gNdsStageMPFallLandFloorLoopP1RootYBeforeMilli, gNdsStageMPFallLandFloorLoopP1RootYAfterPhysicsMilli, gNdsStageMPFallLandFloorLoopP1RootYAfterMilli, gNdsStageMPFallLandFloorLoopP1VelYBeforeMilli, gNdsStageMPFallLandFloorLoopP1VelYAfterMilli, gNdsStageMPFallLandFloorLoopSetGroundCallCount, gNdsStageMPFallLandFloorLoopUnsafeCount, gNdsStageMPFallLandFloorLoopWaitOrLandingCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpFallLandFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCeilFloor) {
        $mpCeilFloorCommands = @(
            'printf "STAGE_MPCEIL_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCeilFloorLoopResult, gNdsFighterMarioFoxStageMPCeilFloorLoopSafeResult, gNdsFighterMarioFoxStageMPCeilFloorLoopMask, gNdsFighterMarioFoxStageMPCeilFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPCeilFloorLoopCount',
            'printf "STAGE_MPCEIL_FLOOR_SETUP=%u,%u,%u,%d,%u,%u\n", gNdsStageMPCeilFloorLoopPrepared, gNdsStageMPCeilFloorLoopBaseMPFallLandSeen, gNdsStageMPCeilFloorLoopCeilLineCount, gNdsStageMPCeilFloorLoopSelectedCeilLineID, gNdsStageMPCeilFloorLoopSelectedCeilKind, gNdsStageMPCeilFloorLoopUnsafeCount',
            'printf "STAGE_MPCEIL_FLOOR_CHECK=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCeilFloorLoopCheckCallCount, gNdsStageMPCeilFloorLoopCheckHitCount, gNdsStageMPCeilFloorLoopCheckMissCount, gNdsStageMPCeilFloorLoopLineSweepSameCallCount, gNdsStageMPCeilFloorLoopLineSweepSameHitCount, gNdsStageMPCeilFloorLoopLineSweepSameMissCount, gNdsStageMPCeilFloorLoopLineSweepDiffCallCount, gNdsStageMPCeilFloorLoopLineSweepDiffHitCount, gNdsStageMPCeilFloorLoopLineSweepDiffMissCount',
            'printf "STAGE_MPCEIL_FLOOR_QUERY=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCeilFloorLoopLineSweepVisitCount, gNdsStageMPCeilFloorLoopLineSweepCandidateCount, gNdsStageMPCeilFloorLoopFCCommonCallCount, gNdsStageMPCeilFloorLoopFCCommonHitCount, gNdsStageMPCeilFloorLoopFCCommonMissCount, gNdsStageMPCeilFloorLoopRunAdjustCallCount, gNdsStageMPCeilFloorLoopMaskCurrCeilCount, gNdsStageMPCeilFloorLoopMaskStatCeilCount',
            'printf "STAGE_MPCEIL_FLOOR_POS=%d,%d,%d,%d,%d,%d,%d\n", gNdsStageMPCeilFloorLoopRootYBeforeMilli, gNdsStageMPCeilFloorLoopRootYAfterCheckMilli, gNdsStageMPCeilFloorLoopRootYAfterAdjustMilli, gNdsStageMPCeilFloorLoopPrevTopYMilli, gNdsStageMPCeilFloorLoopTargetTopYMilli, gNdsStageMPCeilFloorLoopCeilYMilli, gNdsStageMPCeilFloorLoopCeilDistMilli'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCeilFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCeilStatusFloor) {
        $mpCeilStatusFloorCommands = @(
            'printf "STAGE_MPCEILSTATUS_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCeilStatusFloorLoopResult, gNdsFighterMarioFoxStageMPCeilStatusFloorLoopSafeResult, gNdsFighterMarioFoxStageMPCeilStatusFloorLoopMask, gNdsFighterMarioFoxStageMPCeilStatusFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPCeilStatusFloorLoopCount',
            'printf "STAGE_MPCEILSTATUS_FLOOR_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCeilStatusFloorLoopPrepared, gNdsStageMPCeilStatusFloorLoopBaseMPCeilSeen, gNdsStageMPCeilStatusFloorLoopMapCallbackCount, gNdsStageMPCeilStatusFloorLoopCheckCeilHeavyCliffCount, gNdsStageMPCeilStatusFloorLoopSpecialCollisionCount, gNdsStageMPCeilStatusFloorLoopCeilCollisionCount, gNdsStageMPCeilStatusFloorLoopCeilAdjustCount, gNdsStageMPCeilStatusFloorLoopCeilHeavyMaskCount, gNdsStageMPCeilStatusFloorLoopStopCeilSetStatusCount, gNdsStageMPCeilStatusFloorLoopUnsafeCount',
            'printf "STAGE_MPCEILSTATUS_FLOOR_STATUS=%d,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCeilStatusFloorLoopSelectedCeilLineID, gNdsStageMPCeilStatusFloorLoopSelectedCeilKind, gNdsStageMPCeilStatusFloorLoopStatusBefore, gNdsStageMPCeilStatusFloorLoopMotionBefore, gNdsStageMPCeilStatusFloorLoopGABefore, gNdsStageMPCeilStatusFloorLoopStatusAfter, gNdsStageMPCeilStatusFloorLoopMotionAfter, gNdsStageMPCeilStatusFloorLoopGAAfter, gNdsStageMPCeilStatusFloorLoopFtMainSetStatusCount, gNdsStageMPCeilStatusFloorLoopPlayAnimEventsCount',
            'printf "STAGE_MPCEILSTATUS_FLOOR_POS=%d,%d,%d,%d,%#x,%#x\n", gNdsStageMPCeilStatusFloorLoopRootYBeforeMilli, gNdsStageMPCeilStatusFloorLoopRootYAfterMilli, gNdsStageMPCeilStatusFloorLoopVelYBeforeMilli, gNdsStageMPCeilStatusFloorLoopVelYAfterMilli, gNdsStageMPCeilStatusFloorLoopMaskCurr, gNdsStageMPCeilStatusFloorLoopMaskStat'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCeilStatusFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffCatchFloor) {
        $mpCliffCatchFloorCommands = @(
            'printf "STAGE_MPCLIFFCATCH_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffCatchFloorLoopResult, gNdsFighterMarioFoxStageMPCliffCatchFloorLoopSafeResult, gNdsFighterMarioFoxStageMPCliffCatchFloorLoopMask, gNdsFighterMarioFoxStageMPCliffCatchFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffCatchFloorLoopCount',
            'printf "STAGE_MPCLIFFCATCH_FLOOR_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffCatchFloorLoopPrepared, gNdsStageMPCliffCatchFloorLoopBaseMPCeilStatusSeen, gNdsStageMPCliffCatchFloorLoopMapCallbackCount, gNdsStageMPCliffCatchFloorLoopCheckCeilHeavyCliffCount, gNdsStageMPCliffCatchFloorLoopSpecialCollisionCount, gNdsStageMPCliffCatchFloorLoopLCliffTestCount, gNdsStageMPCliffCatchFloorLoopRCliffTestCount, gNdsStageMPCliffCatchFloorLoopLCliffHitCount, gNdsStageMPCliffCatchFloorLoopRCliffHitCount, gNdsStageMPCliffCatchFloorLoopLandingParamCount, gNdsStageMPCliffCatchFloorLoopCliffCatchSetStatusCount, gNdsStageMPCliffCatchFloorLoopFtMainSetStatusCount, gNdsStageMPCliffCatchFloorLoopPlayAnimEventsCount, gNdsStageMPCliffCatchFloorLoopUnsafeCount',
            'printf "STAGE_MPCLIFFCATCH_FLOOR_EFFECTS=%u,%u\n", gNdsStageMPCliffCatchFloorLoopFlashCount, gNdsStageMPCliffCatchFloorLoopCaptureImmuneCount',
            'printf "STAGE_MPCLIFFCATCH_FLOOR_STATUS=%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d\n", gNdsStageMPCliffCatchFloorLoopSelectedLineID, gNdsStageMPCliffCatchFloorLoopSelectedSide, gNdsStageMPCliffCatchFloorLoopStatusBefore, gNdsStageMPCliffCatchFloorLoopMotionBefore, gNdsStageMPCliffCatchFloorLoopGABefore, gNdsStageMPCliffCatchFloorLoopStatusAfter, gNdsStageMPCliffCatchFloorLoopMotionAfter, gNdsStageMPCliffCatchFloorLoopGAAfter, gNdsStageMPCliffCatchFloorLoopIsCliffHoldAfter, gNdsStageMPCliffCatchFloorLoopStopVelCount, gNdsStageMPCliffCatchFloorLoopPhysicsCount, gNdsStageMPCliffCatchFloorLoopCliffIDAfter, gNdsStageMPCliffCatchFloorLoopLRBefore',
            'printf "STAGE_MPCLIFFCATCH_FLOOR_POS=%d,%d,%d,%d,%d,%d,%#x,%#x\n", gNdsStageMPCliffCatchFloorLoopLedgeXMilli, gNdsStageMPCliffCatchFloorLoopLedgeYMilli, gNdsStageMPCliffCatchFloorLoopRootXBeforeMilli, gNdsStageMPCliffCatchFloorLoopRootYBeforeMilli, gNdsStageMPCliffCatchFloorLoopRootXAfterMilli, gNdsStageMPCliffCatchFloorLoopRootYAfterMilli, gNdsStageMPCliffCatchFloorLoopMaskCurr, gNdsStageMPCliffCatchFloorLoopMaskStat',
            'printf "STAGE_MPCLIFFCATCH_FLOOR_OCC=%u,%u,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffCatchFloorLoopOccupancyProbeCount, gNdsStageMPCliffCatchFloorLoopOccupancyBlockCount, gNdsStageMPCliffCatchFloorLoopOccupancyHolderCliffID, gNdsStageMPCliffCatchFloorLoopOccupancyProbeCliffID, gNdsStageMPCliffCatchFloorLoopOccupancyHolderLR, gNdsStageMPCliffCatchFloorLoopOccupancyProbeLR, gNdsStageMPCliffCatchFloorLoopOccupancyStatusAfter, gNdsStageMPCliffCatchFloorLoopOccupancyMotionAfter, gNdsStageMPCliffCatchFloorLoopOccupancyGAAfter, gNdsStageMPCliffCatchFloorLoopOccupancyIsCliffHoldAfter, gNdsStageMPCliffCatchFloorLoopOccupancySetStatusDelta, gNdsStageMPCliffCatchFloorLoopOccupancyLandingParamDelta'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffCatchFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffWaitFloor) {
        $mpCliffWaitFloorCommands = @(
            'printf "STAGE_MPCLIFFWAIT_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult, gNdsFighterMarioFoxStageMPCliffWaitFloorLoopSafeResult, gNdsFighterMarioFoxStageMPCliffWaitFloorLoopMask, gNdsFighterMarioFoxStageMPCliffWaitFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffWaitFloorLoopCount',
            'printf "STAGE_MPCLIFFWAIT_FLOOR_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffWaitFloorLoopPrepared, gNdsStageMPCliffWaitFloorLoopBaseMPCliffCatchSeen, gNdsStageMPCliffWaitFloorLoopCatchUpdateCallCount, gNdsStageMPCliffWaitFloorLoopAnimEndCheckCount, gNdsStageMPCliffWaitFloorLoopAnimEndSetStatusCount, gNdsStageMPCliffWaitFloorLoopCliffWaitSetStatusCount, gNdsStageMPCliffWaitFloorLoopFtMainSetStatusCount, gNdsStageMPCliffWaitFloorLoopPlayerTagWaitCount, gNdsStageMPCliffWaitFloorLoopCaptureImmuneCount, gNdsStageMPCliffWaitFloorLoopInterruptCallCount, gNdsStageMPCliffWaitFloorLoopAttackCheckCount, gNdsStageMPCliffWaitFloorLoopEscapeCheckCount, gNdsStageMPCliffWaitFloorLoopClimbOrFallCheckCount, gNdsStageMPCliffWaitFloorLoopUnsafeCount',
            'printf "STAGE_MPCLIFFWAIT_FLOOR_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsStageMPCliffWaitFloorLoopStatusBefore, gNdsStageMPCliffWaitFloorLoopMotionBefore, gNdsStageMPCliffWaitFloorLoopGABefore, gNdsStageMPCliffWaitFloorLoopStatusAfterUpdate, gNdsStageMPCliffWaitFloorLoopMotionAfterUpdate, gNdsStageMPCliffWaitFloorLoopGAAfterUpdate, gNdsStageMPCliffWaitFloorLoopStatusAfterInterrupt, gNdsStageMPCliffWaitFloorLoopMotionAfterInterrupt, gNdsStageMPCliffWaitFloorLoopGAAfterInterrupt, gNdsStageMPCliffWaitFloorLoopIsCliffHoldAfterUpdate, gNdsStageMPCliffWaitFloorLoopAllowInterruptAfterUpdate, gNdsStageMPCliffWaitFloorLoopCliffIDAfterUpdate, gNdsStageMPCliffWaitFloorLoopLRAfterUpdate, gNdsStageMPCliffWaitFloorLoopFallWaitAfterUpdate, gNdsStageMPCliffWaitFloorLoopFallWaitBeforeInterrupt, gNdsStageMPCliffWaitFloorLoopFallWaitAfterInterrupt, gNdsStageMPCliffWaitFloorLoopPlayerTagWaitAfterUpdate, gNdsStageMPCliffWaitFloorLoopCaptureMaskAfterUpdate, gNdsStageMPCliffWaitFloorLoopProcDamageSetAfterUpdate, gNdsStageMPCliffWaitFloorLoopDamageFallCallCount, gNdsStageMPCliffCatchFloorLoopCliffIDAfter'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffWaitFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffAttackFloor) {
        $mpCliffAttackFloorCommands = @(
            'printf "STAGE_MPCLIFFATTACK_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffAttackFloorLoopResult, gNdsFighterMarioFoxStageMPCliffAttackFloorLoopSafeResult, gNdsFighterMarioFoxStageMPCliffAttackFloorLoopMask, gNdsFighterMarioFoxStageMPCliffAttackFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffAttackFloorLoopCount',
            'printf "STAGE_MPCLIFFATTACK_FLOOR_CALLS=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffAttackFloorLoopPrepared, gNdsStageMPCliffAttackFloorLoopBaseMPCliffWaitSeen, gNdsStageMPCliffAttackFloorLoopInterruptCallCount, gNdsStageMPCliffAttackFloorLoopAttackCheckCount, gNdsStageMPCliffAttackFloorLoopEscapeCheckCount, gNdsStageMPCliffAttackFloorLoopClimbOrFallCheckCount, gNdsStageMPCliffAttackFloorLoopQuickStatusSetCount, gNdsStageMPCliffAttackFloorLoopAnimEventsCount',
            'printf "STAGE_MPCLIFFATTACK_FLOOR_STATUS=%u,%u,%u,%u,%u,%u,%d,%d,%u,%d,%u,%u,%u,%#x,%#x,%d,%d,%u\n", gNdsStageMPCliffAttackFloorLoopStatusBefore, gNdsStageMPCliffAttackFloorLoopMotionBefore, gNdsStageMPCliffAttackFloorLoopGABefore, gNdsStageMPCliffAttackFloorLoopStatusAfter, gNdsStageMPCliffAttackFloorLoopMotionAfter, gNdsStageMPCliffAttackFloorLoopGAAfter, gNdsStageMPCliffAttackFloorLoopCliffIDBefore, gNdsStageMPCliffAttackFloorLoopCliffIDAfter, gNdsStageMPCliffAttackFloorLoopQueuedStatusID, gNdsStageMPCliffAttackFloorLoopQueuedCliffID, gNdsStageMPCliffAttackFloorLoopIsCliffHoldAfter, gNdsStageMPCliffAttackFloorLoopAllowInterruptBefore, gNdsStageMPCliffAttackFloorLoopAllowInterruptAfter, gNdsStageMPCliffAttackFloorLoopButtonTapMask, gNdsStageMPCliffAttackFloorLoopButtonMaskA, gNdsStageMPCliffAttackFloorLoopFallWaitBefore, gNdsStageMPCliffAttackFloorLoopFallWaitAfter, gNdsStageMPCliffAttackFloorLoopDamageFallCallCount',
            'printf "STAGE_MPCLIFFATTACK_FLOOR_SAFE=%u,%#x,%u,%d\n", gNdsStageMPCliffAttackFloorLoopUnsafeCount, gNdsStageMPCliffAttackFloorLoopButtonMaskB, gNdsStageMPCliffAttackFloorLoopProcDamageSetAfter, gNdsStageMPCliffWaitFloorLoopFallWaitAfterInterrupt'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffAttackFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffClimbFloor) {
        $mpCliffClimbFloorCommands = @(
            'printf "STAGE_MPCLIFFCLIMB_FLOOR=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffClimbFloorLoopResult, gNdsFighterMarioFoxStageMPCliffClimbFloorLoopSafeResult, gNdsFighterMarioFoxStageMPCliffClimbFloorLoopMask, gNdsFighterMarioFoxStageMPCliffClimbFloorLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffClimbFloorLoopCount',
            'printf "STAGE_MPCLIFFCLIMB_FLOOR_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffClimbFloorLoopPrepared, gNdsStageMPCliffClimbFloorLoopBaseMPCliffWaitSeen, gNdsStageMPCliffClimbFloorLoopInterruptCallCount, gNdsStageMPCliffClimbFloorLoopAttackCheckCount, gNdsStageMPCliffClimbFloorLoopEscapeCheckCount, gNdsStageMPCliffClimbFloorLoopClimbOrFallCheckCount, gNdsStageMPCliffClimbFloorLoopQuickStatusSetCount, gNdsStageMPCliffClimbFloorLoopFallStatusSetCount, gNdsStageMPCliffClimbFloorLoopAnimEventsCount, gNdsStageMPCliffClimbFloorLoopUnsafeCount',
            'printf "STAGE_MPCLIFFCLIMB_FLOOR_BASE=%u,%u,%u,%d,%d,%d,%u\n", gNdsStageMPCliffClimbFloorLoopStatusBefore, gNdsStageMPCliffClimbFloorLoopMotionBefore, gNdsStageMPCliffClimbFloorLoopGABefore, gNdsStageMPCliffClimbFloorLoopCliffIDBefore, gNdsStageMPCliffClimbFloorLoopLRBefore, gNdsStageMPCliffClimbFloorLoopFallWaitBefore, gNdsStageMPCliffClimbFloorLoopAllowInterruptBefore',
            'printf "STAGE_MPCLIFFCLIMB_FLOOR_CLIMB=%d,%d,%u,%u,%u,%d,%u,%d,%u,%u,%u,%d\n", gNdsStageMPCliffClimbFloorLoopClimbStickX, gNdsStageMPCliffClimbFloorLoopClimbStickY, gNdsStageMPCliffClimbFloorLoopClimbStatusAfter, gNdsStageMPCliffClimbFloorLoopClimbMotionAfter, gNdsStageMPCliffClimbFloorLoopClimbGAAfter, gNdsStageMPCliffClimbFloorLoopClimbCliffIDAfter, gNdsStageMPCliffClimbFloorLoopQueuedStatusID, gNdsStageMPCliffClimbFloorLoopQueuedCliffID, gNdsStageMPCliffClimbFloorLoopClimbIsCliffHoldAfter, gNdsStageMPCliffClimbFloorLoopClimbProcDamageSetAfter, gNdsStageMPCliffClimbFloorLoopClimbAllowInterruptAfter, gNdsStageMPCliffClimbFloorLoopClimbFallWaitAfter',
            'printf "STAGE_MPCLIFFCLIMB_FLOOR_DROP=%d,%d,%u,%u,%u,%d,%d,%d,%u,%u,%u,%u\n", gNdsStageMPCliffClimbFloorLoopDropStickX, gNdsStageMPCliffClimbFloorLoopDropStickY, gNdsStageMPCliffClimbFloorLoopDropStatusAfter, gNdsStageMPCliffClimbFloorLoopDropMotionAfter, gNdsStageMPCliffClimbFloorLoopDropGAAfter, gNdsStageMPCliffClimbFloorLoopDropCliffIDAfter, gNdsStageMPCliffClimbFloorLoopDropFallWaitAfter, gNdsStageMPCliffClimbFloorLoopDropCliffCatchWaitAfter, gNdsStageMPCliffClimbFloorLoopDropIsCliffHoldAfter, gNdsStageMPCliffClimbFloorLoopDropProcDamageSetAfter, gNdsStageMPCliffClimbFloorLoopDropProcCallbacksSetAfter, gNdsStageMPCliffClimbFloorLoopDamageFallCallCount',
            'printf "STAGE_MPCLIFFCLIMB_FLOOR_RECATCH=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffClimbFloorLoopRecatchProbeCount, gNdsStageMPCliffClimbFloorLoopRecatchMapCallbackCount, gNdsStageMPCliffClimbFloorLoopRecatchCheckCeilHeavyCliffCount, gNdsStageMPCliffClimbFloorLoopRecatchSpecialCollisionCount, gNdsStageMPCliffClimbFloorLoopRecatchLCliffTestCount, gNdsStageMPCliffClimbFloorLoopRecatchLCliffHitCount, gNdsStageMPCliffClimbFloorLoopRecatchRCliffTestCount, gNdsStageMPCliffClimbFloorLoopRecatchRCliffHitCount, gNdsStageMPCliffClimbFloorLoopRecatchLandingParamCount, gNdsStageMPCliffClimbFloorLoopRecatchCliffCatchSetStatusCount, gNdsStageMPCliffClimbFloorLoopRecatchFtMainSetStatusCount, gNdsStageMPCliffClimbFloorLoopRecatchOccupancyBlockCount',
            'printf "STAGE_MPCLIFFCLIMB_FLOOR_RECATCH_STATUS=%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%d,%d,%#x,%#x\n", gNdsStageMPCliffClimbFloorLoopRecatchHolderIsCliffHoldBefore, gNdsStageMPCliffClimbFloorLoopRecatchHolderStatusBefore, gNdsStageMPCliffClimbFloorLoopRecatchHolderMotionBefore, gNdsStageMPCliffClimbFloorLoopRecatchHolderGABefore, gNdsStageMPCliffClimbFloorLoopRecatchHolderCliffIDBefore, gNdsStageMPCliffClimbFloorLoopRecatchStatusBefore, gNdsStageMPCliffClimbFloorLoopRecatchMotionBefore, gNdsStageMPCliffClimbFloorLoopRecatchGABefore, gNdsStageMPCliffClimbFloorLoopRecatchStatusAfter, gNdsStageMPCliffClimbFloorLoopRecatchMotionAfter, gNdsStageMPCliffClimbFloorLoopRecatchGAAfter, gNdsStageMPCliffClimbFloorLoopRecatchIsCliffHoldAfter, gNdsStageMPCliffClimbFloorLoopRecatchCliffIDAfter, gNdsStageMPCliffClimbFloorLoopRecatchLRBefore, gNdsStageMPCliffClimbFloorLoopRecatchMaskCurr, gNdsStageMPCliffClimbFloorLoopRecatchMaskStat'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffClimbFloorCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffClimbAction) {
        $mpCliffClimbActionCommands = @(
            'printf "STAGE_MPCLIFFCLIMB_ACTION=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffClimbActionLoopResult, gNdsFighterMarioFoxStageMPCliffClimbActionLoopSafeResult, gNdsFighterMarioFoxStageMPCliffClimbActionLoopMask, gNdsFighterMarioFoxStageMPCliffClimbActionLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffClimbActionLoopCount',
            'printf "STAGE_MPCLIFFCLIMB_ACTION_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffClimbActionLoopPrepared, gNdsStageMPCliffClimbActionLoopBaseMPCliffClimbSeen, gNdsStageMPCliffClimbActionLoopQuickUpdateCallCount, gNdsStageMPCliffClimbActionLoopQuick1SetStatusCount, gNdsStageMPCliffClimbActionLoopQuick1UpdateCallCount, gNdsStageMPCliffClimbActionLoopAnimEndCheckCount, gNdsStageMPCliffClimbActionLoopQuick2SetStatusCount, gNdsStageMPCliffClimbActionLoopCommon2UpdateCollCount, gNdsStageMPCliffClimbActionLoopCommon2InitVarsCount, gNdsStageMPCliffClimbActionLoopUnsafeCount',
            'printf "STAGE_MPCLIFFCLIMB_ACTION_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%u,%d\n", gNdsStageMPCliffClimbActionLoopStatusBefore, gNdsStageMPCliffClimbActionLoopMotionBefore, gNdsStageMPCliffClimbActionLoopGABefore, gNdsStageMPCliffClimbActionLoopStatusAfterQuick1, gNdsStageMPCliffClimbActionLoopMotionAfterQuick1, gNdsStageMPCliffClimbActionLoopGAAfterQuick1, gNdsStageMPCliffClimbActionLoopStatusAfterQuick2, gNdsStageMPCliffClimbActionLoopMotionAfterQuick2, gNdsStageMPCliffClimbActionLoopGAAfterQuick2, gNdsStageMPCliffClimbActionLoopCliffIDBefore, gNdsStageMPCliffClimbActionLoopCliffIDAfterQuick1, gNdsStageMPCliffClimbActionLoopCliffIDAfterQuick2, gNdsStageMPCliffClimbActionLoopFloorLineAfterQuick2, gNdsStageMPCliffClimbActionLoopQueuedStatusID, gNdsStageMPCliffClimbActionLoopQueuedCliffID',
            'printf "STAGE_MPCLIFFCLIMB_ACTION_FLAGS=%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffClimbActionLoopIsCliffHoldAfterQuick1, gNdsStageMPCliffClimbActionLoopIsCliffHoldAfterQuick2, gNdsStageMPCliffClimbActionLoopProcDamageSetAfterQuick1, gNdsStageMPCliffClimbActionLoopProcDamageSetAfterQuick2, gNdsStageMPCliffClimbActionLoopProcUpdateSetAfterQuick1, gNdsStageMPCliffClimbActionLoopProcMapSetAfterQuick2, gNdsStageMPCliffClimbActionLoopJostleIgnoreAfterQuick2',
            'printf "STAGE_MPCLIFFCLIMB_ACTION_ROOT=%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u\n", gNdsStageMPCliffCommon2BridgeCallCount, gNdsStageMPCliffCommon2BridgeGuardPassCount, gNdsStageMPCliffCommon2BridgeGuardRejectCount, gNdsStageMPCliffCommon2BridgeStatusID, gNdsStageMPCliffCommon2BridgeLR, gNdsStageMPCliffCommon2BridgeCliffID, gNdsStageMPCliffCommon2BridgeRootXBeforeMilli, gNdsStageMPCliffCommon2BridgeRootYBeforeMilli, gNdsStageMPCliffCommon2BridgeRootXAfterMilli, gNdsStageMPCliffCommon2BridgeRootYAfterMilli, gNdsStageMPCliffCommon2BridgeExpectedRootXMilli, gNdsStageMPCliffCommon2BridgeExpectedRootYMilli, gNdsStageMPCliffCommon2BridgeFloorDistAfterMilli, gNdsStageMPCliffCommon2BridgeRootPositionOK'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffClimbActionCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffClimbCommon2) {
        $mpCliffClimbCommon2Commands = @(
            'printf "STAGE_MPCLIFFCLIMB_COMMON2=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopResult, gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopSafeResult, gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopMask, gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopDeferredMask, gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopCount',
            'printf "STAGE_MPCLIFFCLIMB_COMMON2_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffClimbCommon2LoopPrepared, gNdsStageMPCliffClimbCommon2LoopBaseMPCliffClimbActionSeen, gNdsStageMPCliffClimbCommon2LoopUpdateCallCount, gNdsStageMPCliffClimbCommon2LoopAnimEndCheckCount, gNdsStageMPCliffClimbCommon2LoopWaitOrFallCallCount, gNdsStageMPCliffClimbCommon2LoopPhysicsCallCount, gNdsStageMPCliffClimbCommon2LoopGroundTransCount, gNdsStageMPCliffClimbCommon2LoopMapCallCount, gNdsStageMPCliffClimbCommon2LoopGroundBreakCount, gNdsStageMPCliffClimbCommon2LoopUnsafeCount',
            'printf "STAGE_MPCLIFFCLIMB_COMMON2_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffClimbCommon2LoopStatusBefore, gNdsStageMPCliffClimbCommon2LoopMotionBefore, gNdsStageMPCliffClimbCommon2LoopGABefore, gNdsStageMPCliffClimbCommon2LoopStatusAfterUpdate, gNdsStageMPCliffClimbCommon2LoopMotionAfterUpdate, gNdsStageMPCliffClimbCommon2LoopGAAfterUpdate, gNdsStageMPCliffClimbCommon2LoopStatusAfterPhysics, gNdsStageMPCliffClimbCommon2LoopMotionAfterPhysics, gNdsStageMPCliffClimbCommon2LoopGAAfterPhysics, gNdsStageMPCliffClimbCommon2LoopStatusAfterMap, gNdsStageMPCliffClimbCommon2LoopMotionAfterMap, gNdsStageMPCliffClimbCommon2LoopGAAfterMap',
            'printf "STAGE_MPCLIFFCLIMB_COMMON2_FLAGS=%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsStageMPCliffClimbCommon2LoopCliffIDBefore, gNdsStageMPCliffClimbCommon2LoopFloorLineBefore, gNdsStageMPCliffClimbCommon2LoopCliffIDAfterMap, gNdsStageMPCliffClimbCommon2LoopFloorLineAfterMap, gNdsStageMPCliffClimbCommon2LoopProcUpdateSet, gNdsStageMPCliffClimbCommon2LoopProcPhysicsSet, gNdsStageMPCliffClimbCommon2LoopProcMapSet, gNdsStageMPCliffClimbCommon2LoopIsCliffHoldAfterMap'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffClimbCommon2Commands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffClimbFinish) {
        $mpCliffClimbFinishCommands = @(
            'printf "STAGE_MPCLIFFCLIMB_FINISH=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffClimbFinishLoopResult, gNdsFighterMarioFoxStageMPCliffClimbFinishLoopSafeResult, gNdsFighterMarioFoxStageMPCliffClimbFinishLoopMask, gNdsFighterMarioFoxStageMPCliffClimbFinishLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffClimbFinishLoopCount',
            'printf "STAGE_MPCLIFFCLIMB_FINISH_CALLS=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffClimbFinishLoopPrepared, gNdsStageMPCliffClimbFinishLoopBaseMPCliffClimbCommon2Seen, gNdsStageMPCliffClimbFinishLoopUpdateCallCount, gNdsStageMPCliffClimbFinishLoopAnimEndCheckCount, gNdsStageMPCliffClimbFinishLoopWaitOrFallCallCount, gNdsStageMPCliffClimbFinishLoopWaitSetStatusCount, gNdsStageMPCliffClimbFinishLoopPlayerTagWaitCount, gNdsStageMPCliffClimbFinishLoopUnsafeCount',
            'printf "STAGE_MPCLIFFCLIMB_FINISH_STATUS=%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffClimbFinishLoopStatusBefore, gNdsStageMPCliffClimbFinishLoopMotionBefore, gNdsStageMPCliffClimbFinishLoopGABefore, gNdsStageMPCliffClimbFinishLoopStatusAfterUpdate, gNdsStageMPCliffClimbFinishLoopMotionAfterUpdate, gNdsStageMPCliffClimbFinishLoopGAAfterUpdate',
            'printf "STAGE_MPCLIFFCLIMB_FINISH_FLAGS=%d,%d,%d,%d,%u,%u,%u,%u,%d,%u,%#x\n", gNdsStageMPCliffClimbFinishLoopCliffIDBefore, gNdsStageMPCliffClimbFinishLoopFloorLineBefore, gNdsStageMPCliffClimbFinishLoopCliffIDAfterUpdate, gNdsStageMPCliffClimbFinishLoopFloorLineAfterUpdate, gNdsStageMPCliffClimbFinishLoopIsCliffHoldAfterUpdate, gNdsStageMPCliffClimbFinishLoopProcUpdateSet, gNdsStageMPCliffClimbFinishLoopProcWaitSet, gNdsStageMPCliffClimbFinishLoopSpecialInterruptAfter, gNdsStageMPCliffClimbFinishLoopPlayerTagWaitAfter, gNdsStageMPCliffClimbFinishLoopIsJostleIgnoreAfterUpdate, gNdsStageMPCliffClimbFinishLoopCommonResetMask'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffClimbFinishCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffWaitDamage) {
        $mpCliffWaitDamageCommands = @(
            'printf "STAGE_MPCLIFFWAIT_DAMAGE=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffWaitDamageLoopResult, gNdsFighterMarioFoxStageMPCliffWaitDamageLoopSafeResult, gNdsFighterMarioFoxStageMPCliffWaitDamageLoopMask, gNdsFighterMarioFoxStageMPCliffWaitDamageLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffWaitDamageLoopCount',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffWaitDamageLoopPrepared, gNdsStageMPCliffWaitDamageLoopBaseMPCliffWaitSeen, gNdsStageMPCliffWaitDamageLoopInterruptCallCount, gNdsStageMPCliffWaitDamageLoopAttackCheckCount, gNdsStageMPCliffWaitDamageLoopEscapeCheckCount, gNdsStageMPCliffWaitDamageLoopClimbOrFallCheckCount, gNdsStageMPCliffWaitDamageLoopDamageFallCallCount, gNdsStageMPCliffWaitDamageLoopSetStatusCount, gNdsStageMPCliffWaitDamageLoopClampRumbleCount, gNdsStageMPCliffWaitDamageLoopCollisionDefaultCount, gNdsStageMPCliffWaitDamageLoopUnsafeCount',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_STATUS=%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffWaitDamageLoopStatusBefore, gNdsStageMPCliffWaitDamageLoopMotionBefore, gNdsStageMPCliffWaitDamageLoopGABefore, gNdsStageMPCliffWaitDamageLoopStatusAfter, gNdsStageMPCliffWaitDamageLoopMotionAfter, gNdsStageMPCliffWaitDamageLoopGAAfter',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_FLAGS=%d,%d,%d,%d,%d,%d,%u,%u,%u,%d,%u\n", gNdsStageMPCliffWaitDamageLoopFallWaitBefore, gNdsStageMPCliffWaitDamageLoopFallWaitAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchWaitAfter, gNdsStageMPCliffWaitDamageLoopCliffIDBefore, gNdsStageMPCliffWaitDamageLoopCliffIDAfter, gNdsStageMPCliffWaitDamageLoopFloorLineAfter, gNdsStageMPCliffWaitDamageLoopIsCliffHoldBefore, gNdsStageMPCliffWaitDamageLoopIsCliffHoldAfter, gNdsStageMPCliffWaitDamageLoopProcDamageSetAfter, gNdsStageMPCliffWaitDamageLoopTicsSinceLastZAfter, gNdsStageMPCliffWaitDamageLoopProcCallbacksSetAfter',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_POS=%d,%d,%d,%d,%d,%d\n", gNdsStageMPCliffWaitDamageLoopRootXBeforeMilli, gNdsStageMPCliffWaitDamageLoopRootYBeforeMilli, gNdsStageMPCliffWaitDamageLoopTargetXMilli, gNdsStageMPCliffWaitDamageLoopTargetYMilli, gNdsStageMPCliffWaitDamageLoopRootXAfterMilli, gNdsStageMPCliffWaitDamageLoopRootYAfterMilli',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_TICK=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%u,%u,%u\n", gNdsStageMPCliffWaitDamageLoopDamageFallInterruptTickCount, gNdsStageMPCliffWaitDamageLoopDamageFallSpecialAirCheckCount, gNdsStageMPCliffWaitDamageLoopDamageFallAttackAirCheckCount, gNdsStageMPCliffWaitDamageLoopDamageFallJumpAerialCheckCount, gNdsStageMPCliffWaitDamageLoopDamageFallHammerCheckCount, gNdsStageMPCliffWaitDamageLoopDamageFallPhysicsTickCount, gNdsStageMPCliffWaitDamageLoopDamageFallMapTickCount, gNdsStageMPCliffWaitDamageLoopDamageFallCliffCheckCount, gNdsStageMPCliffWaitDamageLoopDamageFallNoCollisionCount, gNdsStageMPCliffWaitDamageLoopDamageFallPassiveStandCheckCount, gNdsStageMPCliffWaitDamageLoopDamageFallPassiveCheckCount, gNdsStageMPCliffWaitDamageLoopDamageFallDownBounceSetStatusCount, gNdsStageMPCliffWaitDamageLoopDamageFallVelYBeforeMilli, gNdsStageMPCliffWaitDamageLoopDamageFallVelYAfterMilli, gNdsStageMPCliffWaitDamageLoopDamageFallStatusAfterTick, gNdsStageMPCliffWaitDamageLoopDamageFallMotionAfterTick, gNdsStageMPCliffWaitDamageLoopDamageFallGAAfterTick',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_COLLISION=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u\n", gNdsStageMPCliffWaitDamageLoopDamageFallCollisionHitCount, gNdsStageMPCliffWaitDamageLoopDownBounceMainSetStatusCount, gNdsStageMPCliffWaitDamageLoopDownBounceGroundSetCount, gNdsStageMPCliffWaitDamageLoopDownBounceEffectCount, gNdsStageMPCliffWaitDamageLoopDownBounceSFXCount, gNdsStageMPCliffWaitDamageLoopDownBounceRumbleCount, gNdsStageMPCliffWaitDamageLoopDownBounceVelTransferCount, gNdsStageMPCliffWaitDamageLoopDownBounceStatusAfter, gNdsStageMPCliffWaitDamageLoopDownBounceMotionAfter, gNdsStageMPCliffWaitDamageLoopDownBounceGAAfter, gNdsStageMPCliffWaitDamageLoopDownBounceAttackBufferAfter, gNdsStageMPCliffWaitDamageLoopDownBounceDamageMulMilli, gNdsStageMPCliffWaitDamageLoopDownBounceProcCallbacksSetAfter, gNdsStageMPCliffWaitDamageLoopDownBounceEffectKind, gNdsStageMPCliffWaitDamageLoopDownBounceFGM, gNdsStageMPCliffWaitDamageLoopDownBounceRumbleID',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_CLIFFCATCH=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%#x,%#x,%#x\n", gNdsStageMPCliffWaitDamageLoopDamageFallCliffCatchSetStatusCount, gNdsStageMPCliffWaitDamageLoopCliffCatchMainSetStatusCount, gNdsStageMPCliffWaitDamageLoopCliffCatchGroundSetCount, gNdsStageMPCliffWaitDamageLoopCliffCatchAirSetCount, gNdsStageMPCliffWaitDamageLoopCliffCatchPlayAnimEventsCount, gNdsStageMPCliffWaitDamageLoopCliffCatchStopVelCount, gNdsStageMPCliffWaitDamageLoopCliffCatchFlashCount, gNdsStageMPCliffWaitDamageLoopCliffCatchCaptureImmuneCount, gNdsStageMPCliffWaitDamageLoopCliffCatchStatusAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchMotionAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchGAAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchIsCliffHoldAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchProcDamageSetAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchProcCallbacksSetAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchCliffIDAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchFloorLineAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchMaskStatAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchMaskCurrAfter, gNdsStageMPCliffWaitDamageLoopCliffCatchCaptureMaskAfter',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND=%u,%u,%u,%u,%u,%u,%d,%d,%u\n", gNdsStageMPCliffWaitDamageLoopPassiveStandMainSetStatusCount, gNdsStageMPCliffWaitDamageLoopPassiveStandGroundSetCount, gNdsStageMPCliffWaitDamageLoopPassiveStandVelTransferCount, gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfter, gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfter, gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfter, gNdsStageMPCliffWaitDamageLoopPassiveStandStickX, gNdsStageMPCliffWaitDamageLoopPassiveStandLR, gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfter',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE=%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffWaitDamageLoopPassiveMainSetStatusCount, gNdsStageMPCliffWaitDamageLoopPassiveGroundSetCount, gNdsStageMPCliffWaitDamageLoopPassiveVelTransferCount, gNdsStageMPCliffWaitDamageLoopPassiveStatusAfter, gNdsStageMPCliffWaitDamageLoopPassiveMotionAfter, gNdsStageMPCliffWaitDamageLoopPassiveGAAfter, gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfter',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND_CALLBACKS=%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffWaitDamageLoopPassiveStandPhysicsTickCount, gNdsStageMPCliffWaitDamageLoopPassiveStandMapTickCount, gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfterCallbacks, gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfterCallbacks, gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfterCallbacks, gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfterCallbacks',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE_CALLBACKS=%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffWaitDamageLoopPassivePhysicsTickCount, gNdsStageMPCliffWaitDamageLoopPassiveMapTickCount, gNdsStageMPCliffWaitDamageLoopPassiveStatusAfterCallbacks, gNdsStageMPCliffWaitDamageLoopPassiveMotionAfterCallbacks, gNdsStageMPCliffWaitDamageLoopPassiveGAAfterCallbacks, gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfterCallbacks',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND_TICK=%u,%u,%u,%u,%u,%u,%d,%u\n", gNdsStageMPCliffWaitDamageLoopPassiveStandUpdateTickCount, gNdsStageMPCliffWaitDamageLoopPassiveStandWaitSetStatusCount, gNdsStageMPCliffWaitDamageLoopPassiveStandPlayerTagWaitCount, gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfterUpdate, gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfterUpdate, gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfterUpdate, gNdsStageMPCliffWaitDamageLoopPassiveStandPlayerTagWaitAfterUpdate, gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfterUpdate',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE_TICK=%u,%u,%u,%u,%u,%u,%d,%u\n", gNdsStageMPCliffWaitDamageLoopPassiveUpdateTickCount, gNdsStageMPCliffWaitDamageLoopPassiveWaitSetStatusCount, gNdsStageMPCliffWaitDamageLoopPassivePlayerTagWaitCount, gNdsStageMPCliffWaitDamageLoopPassiveStatusAfterUpdate, gNdsStageMPCliffWaitDamageLoopPassiveMotionAfterUpdate, gNdsStageMPCliffWaitDamageLoopPassiveGAAfterUpdate, gNdsStageMPCliffWaitDamageLoopPassivePlayerTagWaitAfterUpdate, gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfterUpdate',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_DOWNBOUNCE=%u,%u,%u,%u,%u,%d,%u\n", gNdsStageMPCliffWaitDamageLoopDownBounceUpdateTickCount, gNdsStageMPCliffWaitDamageLoopDownBounceAttackCheckCount, gNdsStageMPCliffWaitDamageLoopDownBounceForwardBackCheckCount, gNdsStageMPCliffWaitDamageLoopDownBounceAttackBufferAfterTap, gNdsStageMPCliffWaitDamageLoopDownBounceStatusAfterTap, gNdsStageMPCliffWaitDamageLoopDownBounceMotionAfterTap, gNdsStageMPCliffWaitDamageLoopDownBounceGAAfterTap',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_DOWNWAIT=%u,%u,%u,%d,%u,%d,%u,%d,%u,%u,%d,%d,%u,%u,%d,%u\n", gNdsStageMPCliffWaitDamageLoopDownWaitMainSetStatusCount, gNdsStageMPCliffWaitDamageLoopDownWaitCaptureImmuneCount, gNdsStageMPCliffWaitDamageLoopDownWaitStatusAfter, gNdsStageMPCliffWaitDamageLoopDownWaitMotionAfter, gNdsStageMPCliffWaitDamageLoopDownWaitGAAfter, gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitAfter, gNdsStageMPCliffWaitDamageLoopDownWaitCaptureMaskAfter, gNdsStageMPCliffWaitDamageLoopDownWaitDamageMulMilli, gNdsStageMPCliffWaitDamageLoopDownWaitProcCallbacksSetAfter, gNdsStageMPCliffWaitDamageLoopDownWaitUpdateTickCount, gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitBeforeTick, gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitAfterTick, gNdsStageMPCliffWaitDamageLoopDownWaitStandSetStatusAfterStableTick, gNdsStageMPCliffWaitDamageLoopDownWaitStatusAfterTick, gNdsStageMPCliffWaitDamageLoopDownWaitMotionAfterTick, gNdsStageMPCliffWaitDamageLoopDownWaitGAAfterTick',
            'printf "STAGE_MPCLIFFWAIT_DAMAGE_DOWNSTAND=%u,%u,%u,%d,%u,%u,%u,%u,%d,%d\n", gNdsStageMPCliffWaitDamageLoopDownWaitStandSetStatusCount, gNdsStageMPCliffWaitDamageLoopDownStandMainSetStatusCount, gNdsStageMPCliffWaitDamageLoopDownStandStatusAfterTimeout, gNdsStageMPCliffWaitDamageLoopDownStandMotionAfterTimeout, gNdsStageMPCliffWaitDamageLoopDownStandGAAfterTimeout, gNdsStageMPCliffWaitDamageLoopDownStandFlag1BeforeTimeout, gNdsStageMPCliffWaitDamageLoopDownStandFlag1AfterTimeout, gNdsStageMPCliffWaitDamageLoopDownStandProcCallbacksSetAfterTimeout, gNdsStageMPCliffWaitDamageLoopDownStandStandWaitAfterTimeout, gNdsStageMPCliffWaitDamageLoopDownStandDamageMulMilli'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffWaitDamageCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPPassiveLoop) {
        $mpPassiveLoopCommands = @(
            'printf "STAGE_MPPASSIVE=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPPassiveLoopResult, gNdsFighterMarioFoxStageMPPassiveLoopSafeResult, gNdsFighterMarioFoxStageMPPassiveLoopMask, gNdsFighterMarioFoxStageMPPassiveLoopDeferredMask, gNdsFighterMarioFoxStageMPPassiveLoopCount',
            'printf "STAGE_MPPASSIVE_BRANCH=%#x\n", gNdsStageMPPassiveLoopBranchMask',
            'printf "STAGE_MPPASSIVE_PASSIVESTANDB=%#x\n", gNdsStageMPPassiveLoopPassiveStandBMask',
            'printf "STAGE_MPPASSIVE_NATURALMAP=%#x\n", gNdsStageMPPassiveLoopNaturalMapMask',
            'printf "STAGE_MPPASSIVE_NATURALMAP_CALLS=%u\n", gNdsStageMPPassiveLoopDamageFallMapCallCount',
            'printf "STAGE_MPPASSIVE_SETUP=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopPrepared, gNdsStageMPPassiveLoopBaseDamageSeen, gNdsStageMPPassiveLoopPassiveStandSetStatusCount, gNdsStageMPPassiveLoopPassiveSetStatusCount, gNdsStageMPPassiveLoopPassiveStandGroundSetCount, gNdsStageMPPassiveLoopPassiveGroundSetCount, gNdsStageMPPassiveLoopPassiveStandVelTransferCount, gNdsStageMPPassiveLoopPassiveVelTransferCount, gNdsStageMPPassiveLoopPassiveStandProcCallbacksSetAfterSetup, gNdsStageMPPassiveLoopPassiveProcCallbacksSetAfterSetup, gNdsStageMPPassiveLoopUnsafeCount',
            'printf "STAGE_MPPASSIVE_PASSIVESTAND=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u\n", gNdsStageMPPassiveLoopPassiveStandStatusAfterSetup, gNdsStageMPPassiveLoopPassiveStandMotionAfterSetup, gNdsStageMPPassiveLoopPassiveStandGAAfterSetup, gNdsStageMPPassiveLoopPassiveStandUpdateTickCount, gNdsStageMPPassiveLoopPassiveStandPhysicsTickCount, gNdsStageMPPassiveLoopPassiveStandMapTickCount, gNdsStageMPPassiveLoopPassiveStandStableFrameCount, gNdsStageMPPassiveLoopPassiveStandStatusAfterStable, gNdsStageMPPassiveLoopPassiveStandMotionAfterStable, gNdsStageMPPassiveLoopPassiveStandGAAfterStable, gNdsStageMPPassiveLoopPassiveStandWaitSetStatusCount, gNdsStageMPPassiveLoopPassiveStandPlayerTagWaitAfterFinal, gNdsStageMPPassiveLoopPassiveStandProcCallbacksSetAfterFinal',
            'printf "STAGE_MPPASSIVE_PASSIVE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u\n", gNdsStageMPPassiveLoopPassiveStatusAfterSetup, gNdsStageMPPassiveLoopPassiveMotionAfterSetup, gNdsStageMPPassiveLoopPassiveGAAfterSetup, gNdsStageMPPassiveLoopPassiveUpdateTickCount, gNdsStageMPPassiveLoopPassivePhysicsTickCount, gNdsStageMPPassiveLoopPassiveMapTickCount, gNdsStageMPPassiveLoopPassiveStableFrameCount, gNdsStageMPPassiveLoopPassiveStatusAfterStable, gNdsStageMPPassiveLoopPassiveMotionAfterStable, gNdsStageMPPassiveLoopPassiveGAAfterStable, gNdsStageMPPassiveLoopPassiveWaitSetStatusCount, gNdsStageMPPassiveLoopPassivePlayerTagWaitAfterFinal, gNdsStageMPPassiveLoopPassiveProcCallbacksSetAfterFinal',
            'printf "STAGE_MPPASSIVE_FINAL=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopPassiveStandStatusAfterFinal, gNdsStageMPPassiveLoopPassiveStandMotionAfterFinal, gNdsStageMPPassiveLoopPassiveStandGAAfterFinal, gNdsStageMPPassiveLoopPassiveStatusAfterFinal, gNdsStageMPPassiveLoopPassiveMotionAfterFinal, gNdsStageMPPassiveLoopPassiveGAAfterFinal, gNdsStageMPPassiveLoopPassiveStandPlayerTagWaitCount, gNdsStageMPPassiveLoopPassivePlayerTagWaitCount',
            'printf "STAGE_MPPASSIVE_APPEAL=%#x,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopAppealMask, gNdsStageMPPassiveLoopAppealCheckCount, gNdsStageMPPassiveLoopAppealSetStatusCount, gNdsStageMPPassiveLoopAppealStatusAfter, gNdsStageMPPassiveLoopAppealMotionAfter, gNdsStageMPPassiveLoopAppealGAAfter, gNdsStageMPPassiveLoopAppealProcCallbacksSetAfter, gNdsStageMPPassiveLoopAppealInputButtonTap, gNdsStageMPPassiveLoopAppealInputButtonMaskL',
            'printf "STAGE_MPPASSIVE_APPEAL_GUARD=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopAppealGuardMask, gNdsStageMPPassiveLoopAppealGuardCatchCheckCount, gNdsStageMPPassiveLoopAppealGuardCheckCount, gNdsStageMPPassiveLoopAppealGuardSetStatusCount, gNdsStageMPPassiveLoopAppealGuardStatusAfter, gNdsStageMPPassiveLoopAppealGuardMotionAfter, gNdsStageMPPassiveLoopAppealGuardCallbacksAfter, gNdsStageMPPassiveLoopAppealGuardShieldAfter, gNdsStageMPPassiveLoopAppealGuardInputButtonHold, gNdsStageMPPassiveLoopAppealGuardInputButtonMaskZ',
            'printf "STAGE_MPPASSIVE_CATCH=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopCatchMask, gNdsStageMPPassiveLoopCatchCheckCount, gNdsStageMPPassiveLoopCatchSuccessCount, gNdsStageMPPassiveLoopCatchSetStatusCount, gNdsStageMPPassiveLoopCatchStatusAfter, gNdsStageMPPassiveLoopCatchMotionAfter, gNdsStageMPPassiveLoopCatchGAAfter, gNdsStageMPPassiveLoopCatchCallbacksAfter, gNdsStageMPPassiveLoopCatchParamMaskAfter, gNdsStageMPPassiveLoopCatchItemThrowCheckCount, gNdsStageMPPassiveLoopCatchItemThrowSetStatusCount, gNdsStageMPPassiveLoopCatchPullDeferredCount, gNdsStageMPPassiveLoopCapturePulledDeferredCount',
            'printf "STAGE_MPPASSIVE_CATCH_CALLBACKS=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopCatchCallbackMask, gNdsStageMPPassiveLoopCatchMapTickCount, gNdsStageMPPassiveLoopCatchMapEdgeCheckCount, gNdsStageMPPassiveLoopCatchUpdateTickCount, gNdsStageMPPassiveLoopCatchStatusAfterMap, gNdsStageMPPassiveLoopCatchMotionAfterMap, gNdsStageMPPassiveLoopCatchGAAfterMap, gNdsStageMPPassiveLoopCatchStatusAfterUpdate, gNdsStageMPPassiveLoopCatchMotionAfterUpdate, gNdsStageMPPassiveLoopCatchGAAfterUpdate',
            'printf "STAGE_MPPASSIVE_CATCH_PULL=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopCatchPullMask, gNdsStageMPPassiveLoopCatchPullProcCatchCount, gNdsStageMPPassiveLoopCatchPullSetStatusCount, gNdsStageMPPassiveLoopCatchWaitSetStatusCount, gNdsStageMPPassiveLoopCatchPullUpdateTickCount, gNdsStageMPPassiveLoopCatchWaitInterruptTickCount, gNdsStageMPPassiveLoopCatchWaitThrowCheckCount, gNdsStageMPPassiveLoopCatchPullCaptureImmuneCount, gNdsStageMPPassiveLoopCatchPullEffectCount, gNdsStageMPPassiveLoopCatchPullRumbleCount, gNdsStageMPPassiveLoopCatchPullStatusAfter, gNdsStageMPPassiveLoopCatchPullMotionAfter, gNdsStageMPPassiveLoopCatchPullGAAfter, gNdsStageMPPassiveLoopCatchWaitStatusAfter, gNdsStageMPPassiveLoopCatchWaitMotionAfter, gNdsStageMPPassiveLoopCatchWaitGAAfter, gNdsStageMPPassiveLoopCatchWaitThrowWaitAfterSet, gNdsStageMPPassiveLoopCatchWaitThrowWaitAfterTick, gNdsStageMPPassiveLoopCatchPullSearchGObjReady, gNdsStageMPPassiveLoopCatchPullCatchGObjReady, gNdsStageMPPassiveLoopCatchPullTargetCaptureFlag, gNdsStageMPPassiveLoopUnsafeCount',
            'printf "STAGE_MPPASSIVE_CAPTURE=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%u\n", gNdsStageMPPassiveLoopCaptureMask, gNdsStageMPPassiveLoopCaptureProcCaptureCount, gNdsStageMPPassiveLoopCaptureProcDamageCount, gNdsStageMPPassiveLoopCapturePulledSetStatusCount, gNdsStageMPPassiveLoopCaptureWaitSetStatusCount, gNdsStageMPPassiveLoopCapturePhysicsTickCount, gNdsStageMPPassiveLoopCaptureWaitMapTickCount, gNdsStageMPPassiveLoopCaptureVoiceStopCount, gNdsStageMPPassiveLoopCaptureStopVelCount, gNdsStageMPPassiveLoopCaptureCaptureImmuneCount, gNdsStageMPPassiveLoopCapturePulledStatusAfter, gNdsStageMPPassiveLoopCapturePulledMotionAfter, gNdsStageMPPassiveLoopCapturePulledGAAfter, gNdsStageMPPassiveLoopCaptureWaitStatusAfter, gNdsStageMPPassiveLoopCaptureWaitMotionAfter, gNdsStageMPPassiveLoopCaptureWaitGAAfter, gNdsStageMPPassiveLoopCaptureGObjReady, gNdsStageMPPassiveLoopCaptureLR, gNdsStageMPPassiveLoopCaptureJumpsUsedAfter, gNdsStageMPPassiveLoopCaptureRootXMilli, gNdsStageMPPassiveLoopCaptureRootYMilli, gNdsStageMPPassiveLoopUnsafeCount',
            'printf "STAGE_MPPASSIVE_THROW=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopThrowMask, gNdsStageMPPassiveLoopThrowCheckCount, gNdsStageMPPassiveLoopThrowSetStatusCount, gNdsStageMPPassiveLoopThrowTargetSetStatusCount, gNdsStageMPPassiveLoopThrowAnimEventsCount, gNdsStageMPPassiveLoopThrowCaptureImmuneCount, gNdsStageMPPassiveLoopThrowStatusAfter, gNdsStageMPPassiveLoopThrowMotionAfter, gNdsStageMPPassiveLoopThrowGAAfter, gNdsStageMPPassiveLoopThrowCallbacksAfter, gNdsStageMPPassiveLoopThrowTargetStatusAfter, gNdsStageMPPassiveLoopThrowTargetMotionAfter, gNdsStageMPPassiveLoopThrowTargetGAAfter, gNdsStageMPPassiveLoopThrowTargetJumpsAfter, gNdsStageMPPassiveLoopThrowTargetCaptureGObjReady, gNdsStageMPPassiveLoopUnsafeCount',
            'printf "STAGE_MPPASSIVE_THROW_B=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopThrowBResult, gNdsStageMPPassiveLoopThrowBStatusAfter, gNdsStageMPPassiveLoopThrowBMotionAfter, gNdsStageMPPassiveLoopThrowBGAAfter, gNdsStageMPPassiveLoopThrowBCallbacksAfter, gNdsStageMPPassiveLoopThrowBTargetStatusAfter, gNdsStageMPPassiveLoopThrowBTargetMotionAfter, gNdsStageMPPassiveLoopThrowBTargetGAAfter, gNdsStageMPPassiveLoopThrowBTargetJumpsAfter, gNdsStageMPPassiveLoopThrowBTargetCaptureGObjReady',
            'printf "STAGE_MPPASSIVE_THROW_CALLBACK=%#x,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopThrowCallbackMask, gNdsStageMPPassiveLoopThrowCallbackUpdateCount, gNdsStageMPPassiveLoopThrowCallbackPhysicsCount, gNdsStageMPPassiveLoopThrowCallbackMapCount, gNdsStageMPPassiveLoopThrowCallbackStatusAfterUpdate, gNdsStageMPPassiveLoopThrowCallbackMotionAfterUpdate, gNdsStageMPPassiveLoopThrowCallbackGAAfterUpdate, gNdsStageMPPassiveLoopThrowCallbackFloorLineAfterMap, gNdsStageMPPassiveLoopThrowCallbackCaptureReady, gNdsStageMPPassiveLoopThrowCallbackImmediateUpdateCount, gNdsStageMPPassiveLoopThrowCallbackImmediateSetStatusCount, gNdsStageMPPassiveLoopThrowCallbackImmediateAnimEventsCount, gNdsStageMPPassiveLoopThrowCallbackImmediateCaptureImmuneCount, gNdsStageMPPassiveLoopThrowCallbackImmediateStatusAfter, gNdsStageMPPassiveLoopThrowCallbackImmediateMotionAfter, gNdsStageMPPassiveLoopThrowCallbackImmediateGAAfter, gNdsStageMPPassiveLoopThrowCallbackImmediateJumpsAfter, gNdsStageMPPassiveLoopThrowCallbackImmediateCallbacksAfter',
            'printf "STAGE_MPPASSIVE_THROW_UPDATE=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d\n", gNdsStageMPPassiveLoopThrowUpdateMask, gNdsStageMPPassiveLoopThrowUpdateTickCount, gNdsStageMPPassiveLoopThrowUpdateReleaseCount, gNdsStageMPPassiveLoopThrowUpdateStatusAfter, gNdsStageMPPassiveLoopThrowUpdateMotionAfter, gNdsStageMPPassiveLoopThrowUpdateGAAfter, gNdsStageMPPassiveLoopThrowUpdateCatchCleared, gNdsStageMPPassiveLoopThrowUpdateFlag2After, gNdsStageMPPassiveLoopThrowUpdateCaptureImmuneAfter, gNdsStageMPPassiveLoopThrowUpdateTargetDamageBefore, gNdsStageMPPassiveLoopThrowUpdateTargetDamageAfter, gNdsStageMPPassiveLoopThrowUpdateTargetGAAfter, gNdsStageMPPassiveLoopThrowUpdateTargetCaptureCleared, gNdsStageMPPassiveLoopThrowUpdateTargetProcStatusSet, gNdsStageMPPassiveLoopThrowUpdateReleaseScriptID, gNdsStageMPPassiveLoopThrowUpdateReleaseLR',
            'printf "STAGE_MPPASSIVE_THROW_RELEASE=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d\n", gNdsStageMPPassiveLoopThrowReleaseMask, gNdsStageMPPassiveLoopThrowReleaseUpdateStatsCount, gNdsStageMPPassiveLoopThrowReleaseDamageInitCount, gNdsStageMPPassiveLoopThrowReleaseDamageStatsCount, gNdsStageMPPassiveLoopThrowReleaseDamageUpdateCount, gNdsStageMPPassiveLoopThrowReleasePlayerStatsCount, gNdsStageMPPassiveLoopThrowReleaseStaleQueueCount, gNdsStageMPPassiveLoopThrowReleaseRumbleCount, gNdsStageMPPassiveLoopThrowReleaseCaptureCleared, gNdsStageMPPassiveLoopThrowReleaseGAAfter, gNdsStageMPPassiveLoopThrowReleaseProcStatusSet, gNdsStageMPPassiveLoopThrowReleaseHitStatusAfter, gNdsStageMPPassiveLoopThrowReleaseDamageBefore, gNdsStageMPPassiveLoopThrowReleaseDamageAfter, gNdsStageMPPassiveLoopThrowReleaseDamageInitDamage, gNdsStageMPPassiveLoopThrowReleaseKnockbackMilli, gNdsStageMPPassiveLoopThrowReleaseLR, gNdsStageMPPassiveLoopThrowReleaseScriptID',
            'printf "STAGE_MPPASSIVE_THROW_RELEASE_STATUS=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d\n", gNdsStageMPPassiveLoopThrowReleaseStatusMask, gNdsStageMPPassiveLoopThrowReleaseStatusDamageReleaseCount, gNdsStageMPPassiveLoopThrowReleaseStatusUpdateDamageStatsCount, gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageReleaseCount, gNdsStageMPPassiveLoopThrowReleaseStatusDamageBefore, gNdsStageMPPassiveLoopThrowReleaseStatusDamageAfter, gNdsStageMPPassiveLoopThrowReleaseStatusUpdateBefore, gNdsStageMPPassiveLoopThrowReleaseStatusUpdateAfter, gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageBefore, gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageAfter, gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageQueue, gNdsStageMPPassiveLoopThrowReleaseStatusCaptureCleared, gNdsStageMPPassiveLoopThrowReleaseStatusHitStatusAfter, gNdsStageMPPassiveLoopThrowReleaseStatusLR',
            'printf "STAGE_MPPASSIVE_THROW_PROC_STATUS=%#x,%u,%u,%u,%u,%d\n", gNdsStageMPPassiveLoopThrowProcStatusMask, gNdsStageMPPassiveLoopThrowProcStatusTickCount, gNdsStageMPPassiveLoopThrowProcStatusParamSetCount, gNdsStageMPPassiveLoopThrowProcStatusCaptureReady, gNdsStageMPPassiveLoopThrowProcStatusThrowGObjReady, gNdsStageMPPassiveLoopThrowProcStatusScriptID',
            'printf "STAGE_MPPASSIVE_THROW_DEAD_RESULT=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopThrowDeadResultMask, gNdsStageMPPassiveLoopThrowDeadResultCallCount, gNdsStageMPPassiveLoopThrowDeadResultCollisionCount, gNdsStageMPPassiveLoopThrowDeadResultSetAirCount, gNdsStageMPPassiveLoopThrowDeadResultWaitOrFallCount, gNdsStageMPPassiveLoopThrowDeadResultCatchCleared, gNdsStageMPPassiveLoopThrowDeadResultCaptureCleared, gNdsStageMPPassiveLoopThrowDeadResultFighterStatusAfter, gNdsStageMPPassiveLoopThrowDeadResultTargetStatusAfter, gNdsStageMPPassiveLoopThrowDeadResultFighterGAAfter, gNdsStageMPPassiveLoopThrowDeadResultTargetGAAfter',
            'printf "STAGE_MPPASSIVE_WALLDAMAGE=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopWallDamageMask, gNdsStageMPPassiveLoopWallDamageBaseWallCopySeen, gNdsStageMPPassiveLoopWallDamageCheckCount, gNdsStageMPPassiveLoopWallDamageSetStatusCount, gNdsStageMPPassiveLoopWallDamageImpactWaveCount, gNdsStageMPPassiveLoopWallDamageQuakeCount, gNdsStageMPPassiveLoopWallDamageRumbleCount, gNdsStageMPPassiveLoopWallDamageIntangibleSetCount, gNdsStageMPPassiveLoopWallDamageUpdateTickCount, gNdsStageMPPassiveLoopWallDamageDamageFallCallCount, gNdsStageMPPassiveLoopUnsafeCount',
            'printf "STAGE_MPPASSIVE_WALLDAMAGE_STATE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopWallDamageStatusAfterSetup, gNdsStageMPPassiveLoopWallDamageMotionAfterSetup, gNdsStageMPPassiveLoopWallDamageGAAfterSetup, gNdsStageMPPassiveLoopWallDamageCallbacksAfterSetup, gNdsStageMPPassiveLoopWallDamageStatusAfterUpdate, gNdsStageMPPassiveLoopWallDamageMotionAfterUpdate, gNdsStageMPPassiveLoopWallDamageGAAfterUpdate, gNdsStageMPPassiveLoopWallDamageHitstunBeforeUpdate, gNdsStageMPPassiveLoopWallDamageHitstunAfterUpdate, gNdsStageMPPassiveLoopWallDamageIntangibleAfterSetup',
            'printf "STAGE_MPPASSIVE_WALLDAMAGE_VEC=%d,%d,%d,%d,%d,%d\n", gNdsStageMPPassiveLoopWallDamageVelXBeforeMilli, gNdsStageMPPassiveLoopWallDamageVelXAfterMilli, gNdsStageMPPassiveLoopWallDamageKnockbackMilli, gNdsStageMPPassiveLoopWallDamageLR, gNdsStageMPPassiveLoopWallDamageFloorLineID, gNdsStageMPPassiveLoopWallDamageWallLineID',
            'printf "STAGE_MPPASSIVE_REBOUND=%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPPassiveLoopReboundMask, gNdsStageMPPassiveLoopReboundWaitSetStatusCount, gNdsStageMPPassiveLoopReboundSetStatusCount, gNdsStageMPPassiveLoopReboundWaitUpdateTickCount, gNdsStageMPPassiveLoopReboundUpdateTickCount, gNdsStageMPPassiveLoopReboundFinalWaitSetStatusCount, gNdsStageMPPassiveLoopReboundCallbacksAfterWait, gNdsStageMPPassiveLoopReboundCallbacksAfterSet',
            'printf "STAGE_MPPASSIVE_REBOUND_STATE=%u,%d,%u,%d,%u,%d\n", gNdsStageMPPassiveLoopReboundStatusAfterWait, gNdsStageMPPassiveLoopReboundMotionAfterWait, gNdsStageMPPassiveLoopReboundStatusAfterSet, gNdsStageMPPassiveLoopReboundMotionAfterSet, gNdsStageMPPassiveLoopReboundStatusAfterFinal, gNdsStageMPPassiveLoopReboundMotionAfterFinal',
            'printf "STAGE_MPPASSIVE_REBOUND_VEC=%d,%d,%d,%d,%d,%d\n", gNdsStageMPPassiveLoopReboundVelXMilli, gNdsStageMPPassiveLoopReboundAnimSpeedMilli, gNdsStageMPPassiveLoopReboundTimerAfterSetMilli, gNdsStageMPPassiveLoopReboundTimerAfterFinalMilli, gNdsStageMPPassiveLoopReboundLR, gNdsStageMPPassiveLoopReboundHitLR'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpPassiveLoopCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPDownWaitLoop) {
        $mpDownWaitLoopCommands = @(
            'printf "STAGE_MPDOWNWAIT=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPDownWaitLoopResult, gNdsFighterMarioFoxStageMPDownWaitLoopSafeResult, gNdsFighterMarioFoxStageMPDownWaitLoopMask, gNdsFighterMarioFoxStageMPDownWaitLoopDeferredMask, gNdsFighterMarioFoxStageMPDownWaitLoopCount',
            'printf "STAGE_MPDOWNWAIT_SETUP=%u,%u,%u,%u,%u,%u,%d,%u,%d,%u,%d,%u\n", gNdsStageMPDownWaitLoopPrepared, gNdsStageMPDownWaitLoopBasePassiveSeen, gNdsStageMPDownWaitLoopDownWaitSetStatusCount, gNdsStageMPDownWaitLoopDownWaitMainSetStatusCount, gNdsStageMPDownWaitLoopDownWaitCaptureImmuneCount, gNdsStageMPDownWaitLoopDownWaitStatusAfterSetup, gNdsStageMPDownWaitLoopDownWaitMotionAfterSetup, gNdsStageMPDownWaitLoopDownWaitGAAfterSetup, gNdsStageMPDownWaitLoopDownWaitStandWaitAfterSetup, gNdsStageMPDownWaitLoopDownWaitCaptureMaskAfterSetup, gNdsStageMPDownWaitLoopDownWaitDamageMulMilli, gNdsStageMPDownWaitLoopDownWaitProcCallbacksSetAfterSetup',
            'printf "STAGE_MPDOWNWAIT_INTERRUPT=%u,%u,%u,%u,%u,%u,%#x,%u\n", gNdsStageMPDownWaitLoopInterruptCallCount, gNdsStageMPDownWaitLoopDownAttackCheckCount, gNdsStageMPDownWaitLoopForwardBackCheckCount, gNdsStageMPDownWaitLoopDownStandCheckCount, gNdsStageMPDownWaitLoopDownStandSetStatusCount, gNdsStageMPDownWaitLoopDownStandMainSetStatusCount, gNdsStageMPDownWaitLoopSourceOrder, gNdsStageMPDownWaitLoopUnsafeCount',
            'printf "STAGE_MPDOWNWAIT_INPUT=%d,%d,%u,%u\n", gNdsStageMPDownWaitLoopInputStickX, gNdsStageMPDownWaitLoopInputStickY, gNdsStageMPDownWaitLoopInputButtonTap, gNdsStageMPDownWaitLoopInputButtonMaskZ',
            'printf "STAGE_MPDOWNWAIT_STATUS=%u,%d,%u,%d,%u,%u,%d,%u,%d,%u,%d,%u\n", gNdsStageMPDownWaitLoopStatusBeforeInterrupt, gNdsStageMPDownWaitLoopMotionBeforeInterrupt, gNdsStageMPDownWaitLoopGABeforeInterrupt, gNdsStageMPDownWaitLoopStandWaitBeforeInterrupt, gNdsStageMPDownWaitLoopFlag1BeforeInterrupt, gNdsStageMPDownWaitLoopStatusAfterInterrupt, gNdsStageMPDownWaitLoopMotionAfterInterrupt, gNdsStageMPDownWaitLoopGAAfterInterrupt, gNdsStageMPDownWaitLoopStandWaitAfterInterrupt, gNdsStageMPDownWaitLoopFlag1AfterInterrupt, gNdsStageMPDownWaitLoopDamageMulAfterMilli, gNdsStageMPDownWaitLoopDownStandProcCallbacksSetAfter',
            'printf "STAGE_MPDOWNWAIT_DOWNSTAND_TICK=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%d,%u,%d,%u\n", gNdsStageMPDownWaitLoopDownStandInterruptTickCount, gNdsStageMPDownWaitLoopDownStandKneeBendCheckCount, gNdsStageMPDownWaitLoopDownStandPassCheckCount, gNdsStageMPDownWaitLoopDownStandDokanCheckCount, gNdsStageMPDownWaitLoopDownStandFlag1AfterProcInterrupt, gNdsStageMPDownWaitLoopDownStandUpdateTickCount, gNdsStageMPDownWaitLoopDownStandPhysicsTickCount, gNdsStageMPDownWaitLoopDownStandMapTickCount, gNdsStageMPDownWaitLoopDownStandStableFrameCount, gNdsStageMPDownWaitLoopDownStandStatusAfterStable, gNdsStageMPDownWaitLoopDownStandMotionAfterStable, gNdsStageMPDownWaitLoopDownStandGAAfterStable, gNdsStageMPDownWaitLoopDownStandWaitSetStatusCount, gNdsStageMPDownWaitLoopDownStandPlayerTagWaitCount, gNdsStageMPDownWaitLoopDownStandStatusAfterFinal, gNdsStageMPDownWaitLoopDownStandMotionAfterFinal, gNdsStageMPDownWaitLoopDownStandGAAfterFinal, gNdsStageMPDownWaitLoopDownStandPlayerTagWaitAfterFinal, gNdsStageMPDownWaitLoopDownStandProcCallbacksSetAfterFinal',
            'printf "STAGE_MPDOWNWAIT_ATTACK=%u,%u,%u,%u,%u,%#x,%u,%u,%u,%d,%u,%u,%d,%u,%d,%d,%d,%u\n", gNdsStageMPDownWaitLoopAttackInterruptCallCount, gNdsStageMPDownWaitLoopAttackCheckCount, gNdsStageMPDownWaitLoopAttackSetStatusCount, gNdsStageMPDownWaitLoopAttackMainSetStatusCount, gNdsStageMPDownWaitLoopAttackAnimEventsCount, gNdsStageMPDownWaitLoopAttackSourceOrder, gNdsStageMPDownWaitLoopAttackInputButtonTap, gNdsStageMPDownWaitLoopAttackInputButtonMaskA, gNdsStageMPDownWaitLoopAttackStatusBefore, gNdsStageMPDownWaitLoopAttackMotionBefore, gNdsStageMPDownWaitLoopAttackGABefore, gNdsStageMPDownWaitLoopAttackStatusAfter, gNdsStageMPDownWaitLoopAttackMotionAfter, gNdsStageMPDownWaitLoopAttackGAAfter, gNdsStageMPDownWaitLoopAttackMotionAttackIDAfter, gNdsStageMPDownWaitLoopAttackStatusAttackIDAfter, gNdsStageMPDownWaitLoopAttackStatAttackIDAfter, gNdsStageMPDownWaitLoopAttackProcCallbacksSetAfter',
            'printf "STAGE_MPDOWNWAIT_ATTACK_TICK=%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%d,%u,%d,%u\n", gNdsStageMPDownWaitLoopAttackUpdateTickCount, gNdsStageMPDownWaitLoopAttackPhysicsTickCount, gNdsStageMPDownWaitLoopAttackMapTickCount, gNdsStageMPDownWaitLoopAttackStableFrameCount, gNdsStageMPDownWaitLoopAttackStatusAfterStable, gNdsStageMPDownWaitLoopAttackMotionAfterStable, gNdsStageMPDownWaitLoopAttackGAAfterStable, gNdsStageMPDownWaitLoopAttackWaitSetStatusCount, gNdsStageMPDownWaitLoopAttackPlayerTagWaitCount, gNdsStageMPDownWaitLoopAttackStatusAfterFinal, gNdsStageMPDownWaitLoopAttackMotionAfterFinal, gNdsStageMPDownWaitLoopAttackGAAfterFinal, gNdsStageMPDownWaitLoopAttackPlayerTagWaitAfterFinal, gNdsStageMPDownWaitLoopAttackProcCallbacksSetAfterFinal',
            'printf "STAGE_MPDOWNWAIT_ROLL=%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsStageMPDownWaitLoopRollInterruptCallCount, gNdsStageMPDownWaitLoopRollAttackCheckCount, gNdsStageMPDownWaitLoopRollForwardBackCheckCount, gNdsStageMPDownWaitLoopRollSetStatusCount, gNdsStageMPDownWaitLoopRollMainSetStatusCount, gNdsStageMPDownWaitLoopRollAnimEventsCount, gNdsStageMPDownWaitLoopRollForwardSourceOrder, gNdsStageMPDownWaitLoopRollBackSourceOrder',
            'printf "STAGE_MPDOWNWAIT_ROLL_STATUS=%d,%d,%u,%d,%u,%u,%u,%d,%d,%u,%d,%u,%u,%u\n", gNdsStageMPDownWaitLoopRollForwardInputStickX, gNdsStageMPDownWaitLoopRollForwardInputStickY, gNdsStageMPDownWaitLoopRollForwardStatusAfter, gNdsStageMPDownWaitLoopRollForwardMotionAfter, gNdsStageMPDownWaitLoopRollForwardGAAfter, gNdsStageMPDownWaitLoopRollForwardJostleIgnoreAfter, gNdsStageMPDownWaitLoopRollForwardProcCallbacksSetAfter, gNdsStageMPDownWaitLoopRollBackInputStickX, gNdsStageMPDownWaitLoopRollBackInputStickY, gNdsStageMPDownWaitLoopRollBackStatusAfter, gNdsStageMPDownWaitLoopRollBackMotionAfter, gNdsStageMPDownWaitLoopRollBackGAAfter, gNdsStageMPDownWaitLoopRollBackJostleIgnoreAfter, gNdsStageMPDownWaitLoopRollBackProcCallbacksSetAfter',
            'printf "STAGE_MPDOWNWAIT_ROLL_FORWARD_TICK=%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%d,%u,%d,%u\n", gNdsStageMPDownWaitLoopRollForwardUpdateTickCount, gNdsStageMPDownWaitLoopRollForwardPhysicsTickCount, gNdsStageMPDownWaitLoopRollForwardMapTickCount, gNdsStageMPDownWaitLoopRollForwardStableFrameCount, gNdsStageMPDownWaitLoopRollForwardStatusAfterStable, gNdsStageMPDownWaitLoopRollForwardMotionAfterStable, gNdsStageMPDownWaitLoopRollForwardGAAfterStable, gNdsStageMPDownWaitLoopRollForwardWaitSetStatusCount, gNdsStageMPDownWaitLoopRollForwardPlayerTagWaitCount, gNdsStageMPDownWaitLoopRollForwardStatusAfterFinal, gNdsStageMPDownWaitLoopRollForwardMotionAfterFinal, gNdsStageMPDownWaitLoopRollForwardGAAfterFinal, gNdsStageMPDownWaitLoopRollForwardPlayerTagWaitAfterFinal, gNdsStageMPDownWaitLoopRollForwardProcCallbacksSetAfterFinal',
            'printf "STAGE_MPDOWNWAIT_ROLL_BACK_TICK=%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%d,%u,%d,%u\n", gNdsStageMPDownWaitLoopRollBackUpdateTickCount, gNdsStageMPDownWaitLoopRollBackPhysicsTickCount, gNdsStageMPDownWaitLoopRollBackMapTickCount, gNdsStageMPDownWaitLoopRollBackStableFrameCount, gNdsStageMPDownWaitLoopRollBackStatusAfterStable, gNdsStageMPDownWaitLoopRollBackMotionAfterStable, gNdsStageMPDownWaitLoopRollBackGAAfterStable, gNdsStageMPDownWaitLoopRollBackWaitSetStatusCount, gNdsStageMPDownWaitLoopRollBackPlayerTagWaitCount, gNdsStageMPDownWaitLoopRollBackStatusAfterFinal, gNdsStageMPDownWaitLoopRollBackMotionAfterFinal, gNdsStageMPDownWaitLoopRollBackGAAfterFinal, gNdsStageMPDownWaitLoopRollBackPlayerTagWaitAfterFinal, gNdsStageMPDownWaitLoopRollBackProcCallbacksSetAfterFinal'
            'printf "STAGE_MPDOWNWAIT_ROLL_MOVE=%d,%d,%d,%d,%d,%d,%#x\n", gNdsStageMPDownWaitLoopRollForwardRootXBeforeMilli, gNdsStageMPDownWaitLoopRollForwardRootXAfterStableMilli, gNdsStageMPDownWaitLoopRollForwardRootDeltaXMilli, gNdsStageMPDownWaitLoopRollBackRootXBeforeMilli, gNdsStageMPDownWaitLoopRollBackRootXAfterStableMilli, gNdsStageMPDownWaitLoopRollBackRootDeltaXMilli, gNdsStageMPDownWaitLoopRollMoveMask'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpDownWaitLoopCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageTurnLoop) {
        $stageTurnLoopCommands = @(
            'printf "STAGE_TURN=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageTurnLoopResult, gNdsFighterMarioFoxStageTurnLoopSafeResult, gNdsFighterMarioFoxStageTurnLoopMask, gNdsFighterMarioFoxStageTurnLoopDeferredMask, gNdsFighterMarioFoxStageTurnLoopCount',
            'printf "STAGE_TURN_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageTurnLoopPrepared, gNdsStageTurnLoopBaseDownWaitSeen, gNdsStageTurnLoopCheckCallCount, gNdsStageTurnLoopCheckSuccessCount, gNdsStageTurnLoopSetStatusCount, gNdsStageTurnLoopMainSetStatusCount, gNdsStageTurnLoopAnimEventsCount, gNdsStageTurnLoopUpdateTickCount, gNdsStageTurnLoopFinalUpdateTickCount, gNdsStageTurnLoopPhysicsTickCount, gNdsStageTurnLoopMapTickCount, gNdsStageTurnLoopUnsafeCount',
            'printf "STAGE_TURN_INPUT=%d,%u,%d,%u,%d,%d,%u\n", gNdsStageTurnLoopInputStickX, gNdsStageTurnLoopStatusBefore, gNdsStageTurnLoopMotionBefore, gNdsStageTurnLoopGABefore, gNdsStageTurnLoopLRBefore, gNdsStageTurnLoopVelBeforeMilli, gNdsStageTurnLoopProcCallbacksAfterTurn',
            'printf "STAGE_TURN_SETUP=%u,%d,%u,%d,%d,%u,%u,%u,%d,%d,%d\n", gNdsStageTurnLoopStatusAfterCheck, gNdsStageTurnLoopMotionAfterCheck, gNdsStageTurnLoopGAAfterCheck, gNdsStageTurnLoopLRAfterCheck, gNdsStageTurnLoopVelAfterCheckMilli, gNdsStageTurnLoopAllowAfterSetup, gNdsStageTurnLoopDisableAfterSetup, gNdsStageTurnLoopButtonMaskAfterSetup, gNdsStageTurnLoopLRDashAfterSetup, gNdsStageTurnLoopLRTurnAfterSetup, gNdsStageTurnLoopAttackBufferAfterSetup',
            'printf "STAGE_TURN_UPDATE=%u,%d,%u,%d,%d,%u,%u,%u\n", gNdsStageTurnLoopStatusAfterUpdate, gNdsStageTurnLoopMotionAfterUpdate, gNdsStageTurnLoopGAAfterUpdate, gNdsStageTurnLoopLRAfterUpdate, gNdsStageTurnLoopVelAfterUpdateMilli, gNdsStageTurnLoopFlagAfterUpdate, gNdsStageTurnLoopAllowAfterUpdate, gNdsStageTurnLoopDisableAfterUpdate',
            'printf "STAGE_TURN_FINAL=%u,%d,%u,%d,%d,%u,%u,%d,%u\n", gNdsStageTurnLoopStatusAfterFinal, gNdsStageTurnLoopMotionAfterFinal, gNdsStageTurnLoopGAAfterFinal, gNdsStageTurnLoopLRFinal, gNdsStageTurnLoopVelFinalMilli, gNdsStageTurnLoopWaitSetStatusCount, gNdsStageTurnLoopPlayerTagWaitCount, gNdsStageTurnLoopPlayerTagWaitAfterFinal, gNdsStageTurnLoopProcCallbacksAfterFinal'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $stageTurnLoopCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPDownRecoverLoop) {
        $mpDownRecoverLoopCommands = @(
            'printf "STAGE_MPDOWNRECOVER=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPDownRecoverLoopResult, gNdsFighterMarioFoxStageMPDownRecoverLoopSafeResult, gNdsFighterMarioFoxStageMPDownRecoverLoopMask, gNdsFighterMarioFoxStageMPDownRecoverLoopDeferredMask, gNdsFighterMarioFoxStageMPDownRecoverLoopCount',
            'printf "STAGE_MPDOWNRECOVER_SETUP=%u,%u,%u,%u,%u,%u,%d,%u,%d,%u,%u\n", gNdsStageMPDownRecoverLoopPrepared, gNdsStageMPDownRecoverLoopBaseTurnSeen, gNdsStageMPDownRecoverLoopDownWaitSetStatusCount, gNdsStageMPDownRecoverLoopDownWaitMainSetStatusCount, gNdsStageMPDownRecoverLoopDownWaitCaptureImmuneCount, gNdsStageMPDownRecoverLoopDownWaitStatusAfter, gNdsStageMPDownRecoverLoopDownWaitMotionAfter, gNdsStageMPDownRecoverLoopDownWaitGAAfter, gNdsStageMPDownRecoverLoopDownWaitStandWaitAfter, gNdsStageMPDownRecoverLoopDownWaitCallbacksAfter, gNdsStageMPDownRecoverLoopUnsafeCount',
            'printf "STAGE_MPDOWNRECOVER_DOWNSTAND=%u,%u,%u,%u,%u,%u,%#x,%u,%d,%u,%u\n", gNdsStageMPDownRecoverLoopDownStandInterruptCount, gNdsStageMPDownRecoverLoopDownStandAttackCheckCount, gNdsStageMPDownRecoverLoopDownStandForwardBackCheckCount, gNdsStageMPDownRecoverLoopDownStandCheckCount, gNdsStageMPDownRecoverLoopDownStandSetStatusCount, gNdsStageMPDownRecoverLoopDownStandMainSetStatusCount, gNdsStageMPDownRecoverLoopDownStandSourceOrder, gNdsStageMPDownRecoverLoopDownStandStatusAfter, gNdsStageMPDownRecoverLoopDownStandMotionAfter, gNdsStageMPDownRecoverLoopDownStandGAAfter, gNdsStageMPDownRecoverLoopDownStandCallbacksAfter',
            'printf "STAGE_MPDOWNRECOVER_ATTACK=%u,%u,%u,%u,%u,%#x,%u,%d,%u,%d,%d,%d,%u\n", gNdsStageMPDownRecoverLoopAttackInterruptCount, gNdsStageMPDownRecoverLoopAttackCheckCount, gNdsStageMPDownRecoverLoopAttackSetStatusCount, gNdsStageMPDownRecoverLoopAttackMainSetStatusCount, gNdsStageMPDownRecoverLoopAttackAnimEventsCount, gNdsStageMPDownRecoverLoopAttackSourceOrder, gNdsStageMPDownRecoverLoopAttackStatusAfter, gNdsStageMPDownRecoverLoopAttackMotionAfter, gNdsStageMPDownRecoverLoopAttackGAAfter, gNdsStageMPDownRecoverLoopAttackMotionAttackIDAfter, gNdsStageMPDownRecoverLoopAttackStatusAttackIDAfter, gNdsStageMPDownRecoverLoopAttackStatAttackIDAfter, gNdsStageMPDownRecoverLoopAttackCallbacksAfter',
            'printf "STAGE_MPDOWNRECOVER_ROLL=%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%d,%u,%u,%u,%d,%u,%u\n", gNdsStageMPDownRecoverLoopRollInterruptCount, gNdsStageMPDownRecoverLoopRollAttackCheckCount, gNdsStageMPDownRecoverLoopRollForwardBackCheckCount, gNdsStageMPDownRecoverLoopRollSetStatusCount, gNdsStageMPDownRecoverLoopRollMainSetStatusCount, gNdsStageMPDownRecoverLoopRollAnimEventsCount, gNdsStageMPDownRecoverLoopRollForwardSourceOrder, gNdsStageMPDownRecoverLoopRollBackSourceOrder, gNdsStageMPDownRecoverLoopRollForwardStatusAfter, gNdsStageMPDownRecoverLoopRollForwardMotionAfter, gNdsStageMPDownRecoverLoopRollForwardGAAfter, gNdsStageMPDownRecoverLoopRollForwardCallbacksAfter, gNdsStageMPDownRecoverLoopRollBackStatusAfter, gNdsStageMPDownRecoverLoopRollBackMotionAfter, gNdsStageMPDownRecoverLoopRollBackGAAfter, gNdsStageMPDownRecoverLoopRollBackCallbacksAfter',
            'printf "STAGE_MPDOWNRECOVER_FINAL=%u,%u,%#x,%#x,%#x,%u\n", gNdsStageMPDownRecoverLoopWaitSetStatusCount, gNdsStageMPDownRecoverLoopPlayerTagWaitCount, gNdsStageMPDownRecoverLoopFinalStatusMask, gNdsStageMPDownRecoverLoopFinalMotionMask, gNdsStageMPDownRecoverLoopFinalCallbackMask, gNdsStageMPDownRecoverLoopUnsafeCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpDownRecoverLoopCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffLedgeLoop) {
        $mpCliffLedgeLoopCommands = @(
            'printf "STAGE_MPCLIFFLEDGE=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffLedgeLoopResult, gNdsFighterMarioFoxStageMPCliffLedgeLoopSafeResult, gNdsFighterMarioFoxStageMPCliffLedgeLoopMask, gNdsFighterMarioFoxStageMPCliffLedgeLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffLedgeLoopCount',
            'printf "STAGE_MPCLIFFLEDGE_BASE=%u,%u,%u,%u,%u,%u,%d,%d\n", gNdsStageMPCliffLedgeLoopPrepared, gNdsStageMPCliffLedgeLoopBaseDownRecoverSeen, gNdsStageMPCliffLedgeLoopBaseCliffCatchSeen, gNdsStageMPCliffLedgeLoopBaseCliffClimbSeen, gNdsStageMPCliffLedgeLoopBaseCliffClimbFinishSeen, gNdsStageMPCliffLedgeLoopOccupancyBlockCount, gNdsStageMPCliffLedgeLoopOccupancyHolderCliffID, gNdsStageMPCliffLedgeLoopOccupancyProbeCliffID',
            'printf "STAGE_MPCLIFFLEDGE_STATE=%u,%d,%u,%d,%d,%u,%u,%u,%d,%u,%d,%u,%u,%u,%d,%u,%u\n", gNdsStageMPCliffLedgeLoopDropStatusAfter, gNdsStageMPCliffLedgeLoopDropMotionAfter, gNdsStageMPCliffLedgeLoopDropGAAfter, gNdsStageMPCliffLedgeLoopDropCliffIDAfter, gNdsStageMPCliffLedgeLoopDropCliffCatchWaitAfter, gNdsStageMPCliffLedgeLoopDropIsCliffHoldAfter, gNdsStageMPCliffLedgeLoopDropCallbacksAfter, gNdsStageMPCliffLedgeLoopRecatchStatusAfter, gNdsStageMPCliffLedgeLoopRecatchMotionAfter, gNdsStageMPCliffLedgeLoopRecatchGAAfter, gNdsStageMPCliffLedgeLoopRecatchCliffIDAfter, gNdsStageMPCliffLedgeLoopRecatchIsCliffHoldAfter, gNdsStageMPCliffLedgeLoopRecatchOccupancyBlockCount, gNdsStageMPCliffLedgeLoopClimbFinishStatusAfter, gNdsStageMPCliffLedgeLoopClimbFinishMotionAfter, gNdsStageMPCliffLedgeLoopClimbFinishGAAfter, gNdsStageMPCliffLedgeLoopUnsafeCount',
            'printf "STAGE_MPCLIFFLEDGE_LINES=%d,%d,%d\n", gNdsStageMPCliffLedgeLoopDropCliffIDAfter, gNdsStageMPCliffLedgeLoopClimbFinishCliffIDBefore, gNdsStageMPCliffLedgeLoopClimbFinishFloorLineAfter'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffLedgeLoopCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffLiveLoop) {
        $mpCliffLiveLoopCommands = @(
            'printf "STAGE_MPCLIFFLIVE=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffLiveLoopResult, gNdsFighterMarioFoxStageMPCliffLiveLoopSafeResult, gNdsFighterMarioFoxStageMPCliffLiveLoopMask, gNdsFighterMarioFoxStageMPCliffLiveLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffLiveLoopCount',
            'printf "STAGE_MPCLIFFLIVE_BASE=%u,%u,%u,%u,%u,%#x\n", gNdsStageMPCliffLiveLoopPrepared, gNdsStageMPCliffLiveLoopBaseCliffLedgeSeen, gNdsStageMPCliffLiveLoopProcessAttachCount, gNdsStageMPCliffLiveLoopGObjProcessRunCount, gNdsStageMPCliffLiveLoopCallbackCount, gNdsStageMPCliffLiveLoopCallbackSourceMask',
            'printf "STAGE_MPCLIFFLIVE_COUNTS=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffLiveLoopWaitUpdateCount, gNdsStageMPCliffLiveLoopClimbInterruptCount, gNdsStageMPCliffLiveLoopDropInterruptCount, gNdsStageMPCliffLiveLoopQuickUpdateCount, gNdsStageMPCliffLiveLoopQuick1UpdateCount, gNdsStageMPCliffLiveLoopCommon2UpdateCount, gNdsStageMPCliffLiveLoopCommon2PhysicsCount, gNdsStageMPCliffLiveLoopCommon2MapCount, gNdsStageMPCliffLiveLoopFinishUpdateCount',
            'printf "STAGE_MPCLIFFLIVE_STATUS=%u,%u,%u,%u,%u,%u,%u,%d,%d,%u,%u,%u,%u,%d,%u,%u,%u,%u,%d\n", gNdsStageMPCliffLiveLoopStartStatus, gNdsStageMPCliffLiveLoopStartMotion, gNdsStageMPCliffLiveLoopStartGA, gNdsStageMPCliffLiveLoopWaitStatus, gNdsStageMPCliffLiveLoopWaitMotion, gNdsStageMPCliffLiveLoopWaitGA, gNdsStageMPCliffLiveLoopWaitAllowInterrupt, gNdsStageMPCliffLiveLoopWaitFallWait, gNdsStageMPCliffLiveLoopCliffID, gNdsStageMPCliffLiveLoopClimbStatus, gNdsStageMPCliffLiveLoopClimbMotion, gNdsStageMPCliffLiveLoopClimbGA, gNdsStageMPCliffLiveLoopQueuedStatus, gNdsStageMPCliffLiveLoopQueuedCliffID, gNdsStageMPCliffLiveLoopQuick1Status, gNdsStageMPCliffLiveLoopQuick1Motion, gNdsStageMPCliffLiveLoopQuick2Status, gNdsStageMPCliffLiveLoopQuick2Motion, gNdsStageMPCliffLiveLoopQuick2FloorLine',
            'printf "STAGE_MPCLIFFLIVE_FINAL=%u,%u,%u,%u,%u,%u,%d,%u,%u,%u\n", gNdsStageMPCliffLiveLoopQuick2GA, gNdsStageMPCliffLiveLoopCommon2Status, gNdsStageMPCliffLiveLoopCommon2Motion, gNdsStageMPCliffLiveLoopCommon2GA, gNdsStageMPCliffLiveLoopFinishStatus, gNdsStageMPCliffLiveLoopFinishMotion, gNdsStageMPCliffLiveLoopFinishGA, gNdsStageMPCliffLiveLoopDropStatus, gNdsStageMPCliffLiveLoopDropMotion, gNdsStageMPCliffLiveLoopDropGA',
            'printf "STAGE_MPCLIFFLIVE_DROP=%d,%u,%u\n", gNdsStageMPCliffLiveLoopDropCliffCatchWait, gNdsStageMPCliffLiveLoopDropIsCliffHold, gNdsStageMPCliffLiveLoopUnsafeCount'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffLiveLoopCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffAttackAction) {
        $mpCliffAttackActionCommands = @(
            'printf "STAGE_MPCLIFFATTACK_ACTION=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffAttackActionLoopResult, gNdsFighterMarioFoxStageMPCliffAttackActionLoopSafeResult, gNdsFighterMarioFoxStageMPCliffAttackActionLoopMask, gNdsFighterMarioFoxStageMPCliffAttackActionLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffAttackActionLoopCount',
            'printf "STAGE_MPCLIFFATTACK_ACTION_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffAttackActionLoopPrepared, gNdsStageMPCliffAttackActionLoopBaseMPCliffAttackSeen, gNdsStageMPCliffAttackActionLoopQuickUpdateCallCount, gNdsStageMPCliffAttackActionLoopQuick1SetStatusCount, gNdsStageMPCliffAttackActionLoopQuick1UpdateCallCount, gNdsStageMPCliffAttackActionLoopAnimEndCheckCount, gNdsStageMPCliffAttackActionLoopQuick2SetStatusCount, gNdsStageMPCliffAttackActionLoopCommon2UpdateCollCount, gNdsStageMPCliffAttackActionLoopCommon2InitVarsCount, gNdsStageMPCliffAttackActionLoopUnsafeCount',
            'printf "STAGE_MPCLIFFATTACK_ACTION_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%u,%d\n", gNdsStageMPCliffAttackActionLoopStatusBefore, gNdsStageMPCliffAttackActionLoopMotionBefore, gNdsStageMPCliffAttackActionLoopGABefore, gNdsStageMPCliffAttackActionLoopStatusAfterQuick1, gNdsStageMPCliffAttackActionLoopMotionAfterQuick1, gNdsStageMPCliffAttackActionLoopGAAfterQuick1, gNdsStageMPCliffAttackActionLoopStatusAfterQuick2, gNdsStageMPCliffAttackActionLoopMotionAfterQuick2, gNdsStageMPCliffAttackActionLoopGAAfterQuick2, gNdsStageMPCliffAttackActionLoopCliffIDBefore, gNdsStageMPCliffAttackActionLoopCliffIDAfterQuick1, gNdsStageMPCliffAttackActionLoopCliffIDAfterQuick2, gNdsStageMPCliffAttackActionLoopFloorLineAfterQuick2, gNdsStageMPCliffAttackActionLoopQueuedStatusID, gNdsStageMPCliffAttackActionLoopQueuedCliffID',
            'printf "STAGE_MPCLIFFATTACK_ACTION_FLAGS=%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffAttackActionLoopIsCliffHoldAfterQuick1, gNdsStageMPCliffAttackActionLoopIsCliffHoldAfterQuick2, gNdsStageMPCliffAttackActionLoopProcDamageSetAfterQuick1, gNdsStageMPCliffAttackActionLoopProcDamageSetAfterQuick2, gNdsStageMPCliffAttackActionLoopProcUpdateSetAfterQuick1, gNdsStageMPCliffAttackActionLoopProcMapSetAfterQuick2, gNdsStageMPCliffAttackActionLoopJostleIgnoreAfterQuick2',
            'printf "STAGE_MPCLIFFATTACK_ACTION_ROOT=%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u\n", gNdsStageMPCliffCommon2BridgeCallCount, gNdsStageMPCliffCommon2BridgeGuardPassCount, gNdsStageMPCliffCommon2BridgeGuardRejectCount, gNdsStageMPCliffCommon2BridgeStatusID, gNdsStageMPCliffCommon2BridgeLR, gNdsStageMPCliffCommon2BridgeCliffID, gNdsStageMPCliffCommon2BridgeRootXBeforeMilli, gNdsStageMPCliffCommon2BridgeRootYBeforeMilli, gNdsStageMPCliffCommon2BridgeRootXAfterMilli, gNdsStageMPCliffCommon2BridgeRootYAfterMilli, gNdsStageMPCliffCommon2BridgeExpectedRootXMilli, gNdsStageMPCliffCommon2BridgeExpectedRootYMilli, gNdsStageMPCliffCommon2BridgeFloorDistAfterMilli, gNdsStageMPCliffCommon2BridgeRootPositionOK'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffAttackActionCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffCommon2) {
        $mpCliffCommon2Commands = @(
            'printf "STAGE_MPCLIFFCOMMON2=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffCommon2LoopResult, gNdsFighterMarioFoxStageMPCliffCommon2LoopSafeResult, gNdsFighterMarioFoxStageMPCliffCommon2LoopMask, gNdsFighterMarioFoxStageMPCliffCommon2LoopDeferredMask, gNdsFighterMarioFoxStageMPCliffCommon2LoopCount',
            'printf "STAGE_MPCLIFFCOMMON2_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffCommon2LoopPrepared, gNdsStageMPCliffCommon2LoopBaseMPCliffAttackActionSeen, gNdsStageMPCliffCommon2LoopUpdateCallCount, gNdsStageMPCliffCommon2LoopAnimEndCheckCount, gNdsStageMPCliffCommon2LoopWaitOrFallCallCount, gNdsStageMPCliffCommon2LoopPhysicsCallCount, gNdsStageMPCliffCommon2LoopGroundTransCount, gNdsStageMPCliffCommon2LoopMapCallCount, gNdsStageMPCliffCommon2LoopEdgeBreakCount, gNdsStageMPCliffCommon2LoopUnsafeCount',
            'printf "STAGE_MPCLIFFCOMMON2_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffCommon2LoopStatusBefore, gNdsStageMPCliffCommon2LoopMotionBefore, gNdsStageMPCliffCommon2LoopGABefore, gNdsStageMPCliffCommon2LoopStatusAfterUpdate, gNdsStageMPCliffCommon2LoopMotionAfterUpdate, gNdsStageMPCliffCommon2LoopGAAfterUpdate, gNdsStageMPCliffCommon2LoopStatusAfterPhysics, gNdsStageMPCliffCommon2LoopMotionAfterPhysics, gNdsStageMPCliffCommon2LoopGAAfterPhysics, gNdsStageMPCliffCommon2LoopStatusAfterMap, gNdsStageMPCliffCommon2LoopMotionAfterMap, gNdsStageMPCliffCommon2LoopGAAfterMap',
            'printf "STAGE_MPCLIFFCOMMON2_FLAGS=%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsStageMPCliffCommon2LoopCliffIDBefore, gNdsStageMPCliffCommon2LoopFloorLineBefore, gNdsStageMPCliffCommon2LoopCliffIDAfterMap, gNdsStageMPCliffCommon2LoopFloorLineAfterMap, gNdsStageMPCliffCommon2LoopProcUpdateSet, gNdsStageMPCliffCommon2LoopProcPhysicsSet, gNdsStageMPCliffCommon2LoopProcMapSet, gNdsStageMPCliffCommon2LoopIsCliffHoldAfterMap'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffCommon2Commands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffEscapeAction) {
        $mpCliffEscapeActionCommands = @(
            'printf "STAGE_MPCLIFFESCAPE_ACTION=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffEscapeActionLoopResult, gNdsFighterMarioFoxStageMPCliffEscapeActionLoopSafeResult, gNdsFighterMarioFoxStageMPCliffEscapeActionLoopMask, gNdsFighterMarioFoxStageMPCliffEscapeActionLoopDeferredMask, gNdsFighterMarioFoxStageMPCliffEscapeActionLoopCount',
            'printf "STAGE_MPCLIFFESCAPE_ACTION_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffEscapeActionLoopPrepared, gNdsStageMPCliffEscapeActionLoopBaseMPCliffWaitSeen, gNdsStageMPCliffEscapeActionLoopInterruptCallCount, gNdsStageMPCliffEscapeActionLoopAttackCheckCount, gNdsStageMPCliffEscapeActionLoopEscapeCheckCount, gNdsStageMPCliffEscapeActionLoopClimbOrFallCheckCount, gNdsStageMPCliffEscapeActionLoopQuickStatusSetCount, gNdsStageMPCliffEscapeActionLoopAnimEventsCount, gNdsStageMPCliffEscapeActionLoopQuickUpdateCallCount, gNdsStageMPCliffEscapeActionLoopQuick1SetStatusCount, gNdsStageMPCliffEscapeActionLoopQuick1UpdateCallCount, gNdsStageMPCliffEscapeActionLoopAnimEndCheckCount, gNdsStageMPCliffEscapeActionLoopQuick2SetStatusCount, gNdsStageMPCliffEscapeActionLoopCommon2UpdateCollCount, gNdsStageMPCliffEscapeActionLoopCommon2InitVarsCount, gNdsStageMPCliffEscapeActionLoopUnsafeCount',
            'printf "STAGE_MPCLIFFESCAPE_ACTION_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffEscapeActionLoopStatusBefore, gNdsStageMPCliffEscapeActionLoopMotionBefore, gNdsStageMPCliffEscapeActionLoopGABefore, gNdsStageMPCliffEscapeActionLoopStatusAfterInterrupt, gNdsStageMPCliffEscapeActionLoopMotionAfterInterrupt, gNdsStageMPCliffEscapeActionLoopGAAfterInterrupt, gNdsStageMPCliffEscapeActionLoopStatusAfterQuick1, gNdsStageMPCliffEscapeActionLoopMotionAfterQuick1, gNdsStageMPCliffEscapeActionLoopGAAfterQuick1, gNdsStageMPCliffEscapeActionLoopStatusAfterQuick2, gNdsStageMPCliffEscapeActionLoopMotionAfterQuick2, gNdsStageMPCliffEscapeActionLoopGAAfterQuick2',
            'printf "STAGE_MPCLIFFESCAPE_ACTION_LEDGE=%d,%d,%d,%d,%d,%u,%d\n", gNdsStageMPCliffEscapeActionLoopCliffIDBefore, gNdsStageMPCliffEscapeActionLoopCliffIDAfterInterrupt, gNdsStageMPCliffEscapeActionLoopCliffIDAfterQuick1, gNdsStageMPCliffEscapeActionLoopCliffIDAfterQuick2, gNdsStageMPCliffEscapeActionLoopFloorLineAfterQuick2, gNdsStageMPCliffEscapeActionLoopQueuedStatusID, gNdsStageMPCliffEscapeActionLoopQueuedCliffID',
            'printf "STAGE_MPCLIFFESCAPE_ACTION_FLAGS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%u,%#x,%#x,%#x\n", gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterInterrupt, gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterQuick1, gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterQuick2, gNdsStageMPCliffEscapeActionLoopAllowInterruptBefore, gNdsStageMPCliffEscapeActionLoopAllowInterruptAfter, gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterInterrupt, gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterQuick1, gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterQuick2, gNdsStageMPCliffEscapeActionLoopProcUpdateSetAfterInterrupt, gNdsStageMPCliffEscapeActionLoopProcUpdateSetAfterQuick1, gNdsStageMPCliffEscapeActionLoopProcMapSetAfterQuick2, gNdsStageMPCliffEscapeActionLoopJostleIgnoreAfterQuick2, gNdsStageMPCliffEscapeActionLoopFallWaitBefore, gNdsStageMPCliffEscapeActionLoopFallWaitAfterInterrupt, gNdsStageMPCliffEscapeActionLoopDamageFallCallCount, gNdsStageMPCliffEscapeActionLoopButtonTapMask, gNdsStageMPCliffEscapeActionLoopButtonMaskZ, gNdsStageMPCliffEscapeActionLoopButtonMaskA',
            'printf "STAGE_MPCLIFFESCAPE_ACTION_ROOT=%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u\n", gNdsStageMPCliffCommon2BridgeCallCount, gNdsStageMPCliffCommon2BridgeGuardPassCount, gNdsStageMPCliffCommon2BridgeGuardRejectCount, gNdsStageMPCliffCommon2BridgeStatusID, gNdsStageMPCliffCommon2BridgeLR, gNdsStageMPCliffCommon2BridgeCliffID, gNdsStageMPCliffCommon2BridgeRootXBeforeMilli, gNdsStageMPCliffCommon2BridgeRootYBeforeMilli, gNdsStageMPCliffCommon2BridgeRootXAfterMilli, gNdsStageMPCliffCommon2BridgeRootYAfterMilli, gNdsStageMPCliffCommon2BridgeExpectedRootXMilli, gNdsStageMPCliffCommon2BridgeExpectedRootYMilli, gNdsStageMPCliffCommon2BridgeFloorDistAfterMilli, gNdsStageMPCliffCommon2BridgeRootPositionOK'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffEscapeActionCommands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    if ($RequireStageMPCliffEscapeCommon2) {
        $mpCliffEscapeCommon2Commands = @(
            'printf "STAGE_MPCLIFFESCAPE_COMMON2=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopResult, gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopSafeResult, gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopMask, gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopDeferredMask, gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopCount',
            'printf "STAGE_MPCLIFFESCAPE_COMMON2_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffEscapeCommon2LoopPrepared, gNdsStageMPCliffEscapeCommon2LoopBaseMPCliffEscapeActionSeen, gNdsStageMPCliffEscapeCommon2LoopUpdateCallCount, gNdsStageMPCliffEscapeCommon2LoopAnimEndCheckCount, gNdsStageMPCliffEscapeCommon2LoopWaitOrFallCallCount, gNdsStageMPCliffEscapeCommon2LoopPhysicsCallCount, gNdsStageMPCliffEscapeCommon2LoopGroundTransCount, gNdsStageMPCliffEscapeCommon2LoopMapCallCount, gNdsStageMPCliffEscapeCommon2LoopEdgeBreakCount, gNdsStageMPCliffEscapeCommon2LoopUnsafeCount',
            'printf "STAGE_MPCLIFFESCAPE_COMMON2_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsStageMPCliffEscapeCommon2LoopStatusBefore, gNdsStageMPCliffEscapeCommon2LoopMotionBefore, gNdsStageMPCliffEscapeCommon2LoopGABefore, gNdsStageMPCliffEscapeCommon2LoopStatusAfterUpdate, gNdsStageMPCliffEscapeCommon2LoopMotionAfterUpdate, gNdsStageMPCliffEscapeCommon2LoopGAAfterUpdate, gNdsStageMPCliffEscapeCommon2LoopStatusAfterPhysics, gNdsStageMPCliffEscapeCommon2LoopMotionAfterPhysics, gNdsStageMPCliffEscapeCommon2LoopGAAfterPhysics, gNdsStageMPCliffEscapeCommon2LoopStatusAfterMap, gNdsStageMPCliffEscapeCommon2LoopMotionAfterMap, gNdsStageMPCliffEscapeCommon2LoopGAAfterMap',
            'printf "STAGE_MPCLIFFESCAPE_COMMON2_FLAGS=%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsStageMPCliffEscapeCommon2LoopCliffIDBefore, gNdsStageMPCliffEscapeCommon2LoopFloorLineBefore, gNdsStageMPCliffEscapeCommon2LoopCliffIDAfterMap, gNdsStageMPCliffEscapeCommon2LoopFloorLineAfterMap, gNdsStageMPCliffEscapeCommon2LoopProcUpdateSet, gNdsStageMPCliffEscapeCommon2LoopProcPhysicsSet, gNdsStageMPCliffEscapeCommon2LoopProcMapSet, gNdsStageMPCliffEscapeCommon2LoopIsCliffHoldAfterMap'
        )
        $gdbCommands = @($gdbCommands[0..($gdbCommands.Count - 3)] + $mpCliffEscapeCommon2Commands + $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)])
    }
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $dashRun = [regex]::Match($gdbStdout, 'DASH_RUN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $dashRunAttackEventPos = [regex]::Match($gdbStdout, 'DASH_RUN_ATTACK_EVENT_POS=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0)')
    $dashRunProcParams = [regex]::Match($gdbStdout, 'DASH_RUN_PROCPARAMS=(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $dashRunProcParamsRumble = [regex]::Match($gdbStdout, 'DASH_RUN_PROCPARAMS_RUMBLE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(-?[0-9]+)')
    $dashRunDamageStatus = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_STATUS=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $dashRunDamageSetup = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_SETUP=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $dashRunDamageDust = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_DUST=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $dashRunDamageDustUpdate = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_DUST_UPDATE=(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+)')
    $dashRunDamageHitstunPublic = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_HITSTUN_PUBLIC=(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+)')
    $dashRunDamageColAnim = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_COLANIM=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $dashRunDamageColAnimUpdate = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_COLANIM_UPDATE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $dashRunDamageInvincible = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_INVINCIBLE=(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+)')
    $dashRunDamageLagUpdate = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_LAGUPDATE=(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+)')
    $dashRunDamageCommonPhysics = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_COMMON_PHYSICS=(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $dashRunDamageCommonCallbacks = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_COMMON_CALLBACKS=(0x[0-9a-fA-F]+|0)')
    $dashRunDamageLevels = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_LEVELS=(0x[0-9a-fA-F]+|0)')
    $dashRunDamageHoldResist = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_HOLD_RESIST=(0x[0-9a-fA-F]+|0)')
    $dashRunDamageUpdateCatchResist = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_UPDATE_CATCH_RESIST=(0x[0-9a-fA-F]+|0)')
    $dashRunDamageAirMapWall = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_AIR_MAP_WALL=(0x[0-9a-fA-F]+|0)')
    $dashRunDamageKnockbackAngle = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_KNOCKBACK_ANGLE=(0x[0-9a-fA-F]+|0)')
    $dashRunDamageFallInterrupt = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_FALL_INTERRUPT=(0x[0-9a-fA-F]+|0)')
    $dashRunDamageScreenFlash = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_SCREEN_FLASH=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $dashRunDamagePublic = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_PUBLIC=(0x[0-9a-fA-F]+|0),(-?[0-9]+),([0-9]+)')
    $dashRunDamageVoice = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_VOICE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $dashRunDamageFlyTop = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_FLYTOP=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $dashRunDamageReplaceElectric = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_REPLACE_ELECTRIC=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $dashRunDamageFlyRoll = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_FLYROLL=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $dashRunDamageKirbyCopy = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_KIRBYCOPY=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $dashRunDamageItemHeavy = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_ITEM_HEAVY=(0x[0-9a-fA-F]+|0)')
    $dashRunDamageItemBypass = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_ITEM_BYPASS=(0x[0-9a-fA-F]+|0)')
    $dashRunDamageKind = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_KIND=(0x[0-9a-fA-F]+|0)')
    $dashRunDamageSleep = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_SLEEP=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $dashRunDamageLoseGrip = [regex]::Match($gdbStdout, 'DASH_RUN_DAMAGE_LOSEGRIP=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPDamageRecover = [regex]::Match($gdbStdout, 'STAGE_MPDAMAGE_RECOVER=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPDamageRecoverSetup = [regex]::Match($gdbStdout, 'STAGE_MPDAMAGE_RECOVER_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPDamageRecoverContact = [regex]::Match($gdbStdout, 'STAGE_MPDAMAGE_RECOVER_CONTACT=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPDamageRecoverDamage = [regex]::Match($gdbStdout, 'STAGE_MPDAMAGE_RECOVER_DAMAGE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPDamageRecoverStatus = [regex]::Match($gdbStdout, 'STAGE_MPDAMAGE_RECOVER_STATUS=([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPDamageRecoverCallbacks = [regex]::Match($gdbStdout, 'STAGE_MPDAMAGE_RECOVER_CALLBACKS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPDamageRecoverGround = [regex]::Match($gdbStdout, 'STAGE_MPDAMAGE_RECOVER_GROUND=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPDamageRecoverBranches = [regex]::Match($gdbStdout, 'STAGE_MPDAMAGE_RECOVER_BRANCHES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPDamageRecoverVel = [regex]::Match($gdbStdout, 'STAGE_MPDAMAGE_RECOVER_VEL=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPDamageRecoverSafe = [regex]::Match($gdbStdout, 'STAGE_MPDAMAGE_RECOVER_SAFE=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitDamage = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_DAMAGE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPLiveHitSetup = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitAttack = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_ATTACK=([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageMPLiveHitEvents = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_EVENTS=([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageMPLiveHitAttackData = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_ATTACKDATA=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageMPLiveHitSecondary = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_SECONDARY=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageMPLiveHitHurtbox = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_HURTBOX=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitHurtboxDamage = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_HURTBOX_DAMAGE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPLiveHitEffectOnly = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_EFFECTONLY=(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPLiveHitResist = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_RESIST=(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPLiveHitResistBreak = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_RESIST_BREAK=(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPLiveHitThrowAttr = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_THROWATTR=(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPLiveHitAttackClash = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_ATTACK_CLASH=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPLiveHitCatchStat = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_CATCHSTAT=(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitCatchSearch = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_CATCHSEARCH=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPLiveHitPos = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitColl = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_COLL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitRehit = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_REHIT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageMPLiveHitShield = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_SHIELD=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageMPLiveHitShieldSetOffTick = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_SHIELD_SETOFF_TICK=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPLiveHitShieldContact = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_SHIELD_CONTACT=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPLiveHitOriginalRehit = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_ORIG_REHIT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageMPLiveHitOriginalRehitHit = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_ORIG_REHIT_HIT=([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+)')
    $stageMPLiveHitDamageState = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_DAMAGESTATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPLiveHitProc = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_PROC=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitRecover = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_RECOVER=([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitSafe = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_SAFE=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitStatus = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_STATUS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPLiveHitStatusSetup = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_STATUS_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitStatusSearch = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_STATUS_SEARCH=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitStatusProc = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_STATUS_PROC=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitStatusHitlag = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_STATUS_HITLAG=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveHitStatusCallback = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_STATUS_CALLBACK=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageMPLiveHitStatusRepeat = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_STATUS_REPEAT=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageMPLiveHitStatusSafe = [regex]::Match($gdbStdout, 'STAGE_MPLIVEHIT_STATUS_SAFE=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $vs = [regex]::Match($gdbStdout, 'VS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $pv = [regex]::Match($gdbStdout, 'PV_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $maps = [regex]::Match($gdbStdout, 'MAPS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $prev = [regex]::Match($gdbStdout, 'GCRUNALL_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $loop = [regex]::Match($gdbStdout, 'GCDRAWALL_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $taskman = [regex]::Match($gdbStdout, 'GCDRAWALL_TASKMAN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $run = [regex]::Match($gdbStdout, 'GCDRAWALL_RUN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $process = [regex]::Match($gdbStdout, 'GCDRAWALL_PROCESS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'GCDRAWALL_INPUT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $status = [regex]::Match($gdbStdout, 'GCDRAWALL_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $draw = [regex]::Match($gdbStdout, 'GCDRAWALL_DRAW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $screen = [regex]::Match($gdbStdout, 'GCDRAWALL_SCREEN=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $move = [regex]::Match($gdbStdout, 'GCDRAWALL_MOVE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $trans = [regex]::Match($gdbStdout, 'GCDRAWALL_TRANS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'GCDRAWALL_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platform = [regex]::Match($gdbStdout, 'PLATFORM_DL_PREVIEW=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $platformHw = [regex]::Match($gdbStdout, 'PLATFORM_HW=([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stage = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageCapture = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_CAPTURE=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $stageDObj = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_DOBJ=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageSafe = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stagePixels = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_PIXELS=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageHardware = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_HW=([0-9]+),([0-9]+)')
    $stageCollision = [regex]::Match($gdbStdout, 'STAGE_COLLISION=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageCollisionGeom = [regex]::Match($gdbStdout, 'STAGE_COLLISION_GEOM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageCollisionProject = [regex]::Match($gdbStdout, 'STAGE_COLLISION_PROJECT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageCollisionProbes = [regex]::Match($gdbStdout, 'STAGE_COLLISION_PROBES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageCollisionP0 = [regex]::Match($gdbStdout, 'STAGE_COLLISION_P0=(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageCollisionP1 = [regex]::Match($gdbStdout, 'STAGE_COLLISION_P1=(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageCollisionEdge = [regex]::Match($gdbStdout, 'STAGE_COLLISION_EDGE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageCollisionKind = [regex]::Match($gdbStdout, 'STAGE_COLLISION_KIND=(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageCollisionSafe = [regex]::Match($gdbStdout, 'STAGE_COLLISION_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageFloorFollow = [regex]::Match($gdbStdout, 'STAGE_FLOOR_FOLLOW=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageFloorFollowSetup = [regex]::Match($gdbStdout, 'STAGE_FLOOR_FOLLOW_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageFloorFollowUpdates = [regex]::Match($gdbStdout, 'STAGE_FLOOR_FOLLOW_UPDATES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageFloorFollowP0 = [regex]::Match($gdbStdout, 'STAGE_FLOOR_FOLLOW_P0=(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $stageFloorFollowP1 = [regex]::Match($gdbStdout, 'STAGE_FLOOR_FOLLOW_P1=(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $stageFloorFollowDrift = [regex]::Match($gdbStdout, 'STAGE_FLOOR_FOLLOW_DRIFT=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageFloorEdge = [regex]::Match($gdbStdout, 'STAGE_FLOOR_EDGE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageFloorEdgeLine = [regex]::Match($gdbStdout, 'STAGE_FLOOR_EDGE_LINE=([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageFloorEdgeP0 = [regex]::Match($gdbStdout, 'STAGE_FLOOR_EDGE_P0=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageFloorEdgeP1 = [regex]::Match($gdbStdout, 'STAGE_FLOOR_EDGE_P1=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageFloorEdgeProbes = [regex]::Match($gdbStdout, 'STAGE_FLOOR_EDGE_PROBES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageFloorEdgeQueries = [regex]::Match($gdbStdout, 'STAGE_FLOOR_EDGE_QUERIES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageFloorEdgeUpdates = [regex]::Match($gdbStdout, 'STAGE_FLOOR_EDGE_UPDATES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageFloorEdgeSafe = [regex]::Match($gdbStdout, 'STAGE_FLOOR_EDGE_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPProcessFloor = [regex]::Match($gdbStdout, 'STAGE_MPPROCESS_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPProcessFloorAdapter = [regex]::Match($gdbStdout, 'STAGE_MPPROCESS_FLOOR_ADAPTER=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPProcessFloorCalls = [regex]::Match($gdbStdout, 'STAGE_MPPROCESS_FLOOR_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPProcessFloorFC = [regex]::Match($gdbStdout, 'STAGE_MPPROCESS_FLOOR_FC=([0-9]+),([0-9]+),([0-9]+)')
    $stageMPProcessFloorProbes = [regex]::Match($gdbStdout, 'STAGE_MPPROCESS_FLOOR_PROBES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPProcessFloorP0 = [regex]::Match($gdbStdout, 'STAGE_MPPROCESS_FLOOR_P0=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+)')
    $stageMPProcessFloorP1 = [regex]::Match($gdbStdout, 'STAGE_MPPROCESS_FLOOR_P1=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+)')
    $stageMPProcessFloorSafe = [regex]::Match($gdbStdout, 'STAGE_MPPROCESS_FLOOR_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPUpdateFloor = [regex]::Match($gdbStdout, 'STAGE_MPUPDATE_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPUpdateFloorAdapter = [regex]::Match($gdbStdout, 'STAGE_MPUPDATE_FLOOR_ADAPTER=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPUpdateFloorUpdate = [regex]::Match($gdbStdout, 'STAGE_MPUPDATE_FLOOR_UPDATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPUpdateFloorColl = [regex]::Match($gdbStdout, 'STAGE_MPUPDATE_FLOOR_COLL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPUpdateFloorChecks = [regex]::Match($gdbStdout, 'STAGE_MPUPDATE_FLOOR_CHECKS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPUpdateFloorProbes = [regex]::Match($gdbStdout, 'STAGE_MPUPDATE_FLOOR_PROBES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPUpdateFloorP0 = [regex]::Match($gdbStdout, 'STAGE_MPUPDATE_FLOOR_P0=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPUpdateFloorP1 = [regex]::Match($gdbStdout, 'STAGE_MPUPDATE_FLOOR_P1=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPUpdateFloorSafe = [regex]::Match($gdbStdout, 'STAGE_MPUPDATE_FLOOR_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPSweepFloor = [regex]::Match($gdbStdout, 'STAGE_MPSWEEP_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPSweepFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPSWEEP_FLOOR_SETUP=([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPSweepFloorCheck = [regex]::Match($gdbStdout, 'STAGE_MPSWEEP_FLOOR_CHECK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPSweepFloorSweep = [regex]::Match($gdbStdout, 'STAGE_MPSWEEP_FLOOR_SWEEP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPSweepFloorSecond = [regex]::Match($gdbStdout, 'STAGE_MPSWEEP_FLOOR_SECOND=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPSweepFloorProbes = [regex]::Match($gdbStdout, 'STAGE_MPSWEEP_FLOOR_PROBES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPSweepFloorP0 = [regex]::Match($gdbStdout, 'STAGE_MPSWEEP_FLOOR_P0=(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPSweepFloorP1 = [regex]::Match($gdbStdout, 'STAGE_MPSWEEP_FLOOR_P1=(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPCrossFloor = [regex]::Match($gdbStdout, 'STAGE_MPCROSS_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCrossFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPCROSS_FLOOR_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPCrossFloorLive = [regex]::Match($gdbStdout, 'STAGE_MPCROSS_FLOOR_LIVE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCrossFloorP0 = [regex]::Match($gdbStdout, 'STAGE_MPCROSS_FLOOR_P0=([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPCrossFloorP1 = [regex]::Match($gdbStdout, 'STAGE_MPCROSS_FLOOR_P1=([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPAdjustFloor = [regex]::Match($gdbStdout, 'STAGE_MPADJUST_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPAdjustFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPADJUST_FLOOR_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPAdjustFloorCheck = [regex]::Match($gdbStdout, 'STAGE_MPADJUST_FLOOR_CHECK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPAdjustFloorWall = [regex]::Match($gdbStdout, 'STAGE_MPADJUST_FLOOR_WALL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPAdjustFloorEdge = [regex]::Match($gdbStdout, 'STAGE_MPADJUST_FLOOR_EDGE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPAdjustFloorP0P1 = [regex]::Match($gdbStdout, 'STAGE_MPADJUST_FLOOR_P0P1=([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPEdgeFloor = [regex]::Match($gdbStdout, 'STAGE_MPEDGE_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPEdgeFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPEDGE_FLOOR_SETUP=([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPEdgeFloorEdge = [regex]::Match($gdbStdout, 'STAGE_MPEDGE_FLOOR_EDGE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPEdgeFloorLine = [regex]::Match($gdbStdout, 'STAGE_MPEDGE_FLOOR_LINE=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallFloor = [regex]::Match($gdbStdout, 'STAGE_MPWALL_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPWallFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPWALL_FLOOR_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallFloorLine = [regex]::Match($gdbStdout, 'STAGE_MPWALL_FLOOR_LINE=(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallFloorPos = [regex]::Match($gdbStdout, 'STAGE_MPWALL_FLOOR_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPWallHitScout = [regex]::Match($gdbStdout, 'STAGE_MPWALL_HIT_SCOUT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallHitScoutLine = [regex]::Match($gdbStdout, 'STAGE_MPWALL_HIT_SCOUT_LINE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallHitScoutPos = [regex]::Match($gdbStdout, 'STAGE_MPWALL_HIT_SCOUT_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPWallHyruleScout = [regex]::Match($gdbStdout, 'STAGE_MPWALL_HYRULE_SCOUT=(0x[0-9a-fA-F]+|[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallHyruleScoutLine = [regex]::Match($gdbStdout, 'STAGE_MPWALL_HYRULE_SCOUT_LINE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallHyruleScoutPos = [regex]::Match($gdbStdout, 'STAGE_MPWALL_HYRULE_SCOUT_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPWallHitFloor = [regex]::Match($gdbStdout, 'STAGE_MPWALLHIT_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPWallHitFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPWALLHIT_FLOOR_SETUP=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallHitFloorCount = [regex]::Match($gdbStdout, 'STAGE_MPWALLHIT_FLOOR_COUNT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallHitFloorLine = [regex]::Match($gdbStdout, 'STAGE_MPWALLHIT_FLOOR_LINE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallHitFloorPos = [regex]::Match($gdbStdout, 'STAGE_MPWALLHIT_FLOOR_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPWallCopyFloor = [regex]::Match($gdbStdout, 'STAGE_MPWALLCOPY_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPWallCopyFloorBase = [regex]::Match($gdbStdout, 'STAGE_MPWALLCOPY_BASE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallCopyFloorSrc = [regex]::Match($gdbStdout, 'STAGE_MPWALLCOPY_SRC=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPWallCopyFloorPos = [regex]::Match($gdbStdout, 'STAGE_MPWALLCOPY_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPWallCopyFloorState = [regex]::Match($gdbStdout, 'STAGE_MPWALLCOPY_STATE=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPassFloor = [regex]::Match($gdbStdout, 'STAGE_MPPASS_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPPassFloorBase = [regex]::Match($gdbStdout, 'STAGE_MPPASS_BASE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassFloorLine = [regex]::Match($gdbStdout, 'STAGE_MPPASS_LINE=(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPPassFloorRoute = [regex]::Match($gdbStdout, 'STAGE_MPPASS_ROUTE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassFloorProcess = [regex]::Match($gdbStdout, 'STAGE_MPPASS_PROCESS=([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassFloorP1 = [regex]::Match($gdbStdout, 'STAGE_MPPASS_P1=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPPlatformFloor = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPPlatformFloorBase = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_BASE=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPlatformFloorLine = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_LINE=(-?[0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPlatformFloorDObj = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_DOBJ=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageMPPlatformTickFloor = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_TICK=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPPlatformTickFloorStep = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_TICK_STEP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassInputLoop = [regex]::Match($gdbStdout, 'STAGE_MPPASS_INPUT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPPassInputLoopSetup = [regex]::Match($gdbStdout, 'STAGE_MPPASS_INPUT_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassInputLoopState = [regex]::Match($gdbStdout, 'STAGE_MPPASS_INPUT_STATE=(-?[0-9]+),(0x[0-9a-fA-F]+|0),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPassInputLoopSquatRv = [regex]::Match($gdbStdout, 'STAGE_MPPASS_INPUT_SQUATRV=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPlatformPosFloor = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_POS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPPlatformPosFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_POS_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPlatformPosFloorVec = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_POS_VEC=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPlatformSpeedFloor = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_SPEED=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPPlatformSpeedFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_SPEED_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPlatformSpeedFloorVec = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_SPEED_VEC=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPlatformSpeedFloorDynamic = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_SPEED_DYNAMIC=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPlatformSpeedFloorDynamicCeil = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_SPEED_DYNAMIC_CEIL=([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPlatformSpeedFloorDynamicWall = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_SPEED_DYNAMIC_WALL=([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPlatformSpeedFloorProcessWall = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_SPEED_PROCESS_WALL=([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPPlatformSpeedFloorAnim = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_SPEED_ANIM=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPlatformSpeedFloorBounds = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_SPEED_BOUNDS=([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPlatformSpeedFloorStageAnim = [regex]::Match($gdbStdout, 'STAGE_MPPLATFORM_SPEED_STAGE_ANIM=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $stageInishieAsset = [regex]::Match($gdbStdout, 'STAGE_INISHIE_ASSET=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageInishieScale = [regex]::Match($gdbStdout, 'STAGE_INISHIE_SCALE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageInishieScaleSetup = [regex]::Match($gdbStdout, 'STAGE_INISHIE_SCALE_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageInishieScaleSource = [regex]::Match($gdbStdout, 'STAGE_INISHIE_SCALE_SOURCE=([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageInishieScaleDisplay = [regex]::Match($gdbStdout, 'STAGE_INISHIE_SCALE_DISPLAY=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageInishieScaleMaterial = [regex]::Match($gdbStdout, 'STAGE_INISHIE_SCALE_MATERIAL=(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageInishieScalePreview = [regex]::Match($gdbStdout, 'STAGE_INISHIE_SCALE_PREVIEW=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageInishieScaleLines = [regex]::Match($gdbStdout, 'STAGE_INISHIE_SCALE_LINES=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageInishieScaleAlt = [regex]::Match($gdbStdout, 'STAGE_INISHIE_SCALE_ALT=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageInishieScaleFall = [regex]::Match($gdbStdout, 'STAGE_INISHIE_SCALE_FALL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageInishieScaleStep = [regex]::Match($gdbStdout, 'STAGE_INISHIE_SCALE_STEP=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPStaleFloor = [regex]::Match($gdbStdout, 'STAGE_MPSTALE_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPStaleFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPSTALE_FLOOR_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPStaleFloorLive = [regex]::Match($gdbStdout, 'STAGE_MPSTALE_FLOOR_LIVE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPStaleFloorP0P1 = [regex]::Match($gdbStdout, 'STAGE_MPSTALE_FLOOR_P0P1=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveStaleFloor = [regex]::Match($gdbStdout, 'STAGE_MPLIVESTALE_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPLiveStaleFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPLIVESTALE_FLOOR_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveStaleFloorLive = [regex]::Match($gdbStdout, 'STAGE_MPLIVESTALE_FLOOR_LIVE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPLiveStaleFloorP0P1 = [regex]::Match($gdbStdout, 'STAGE_MPLIVESTALE_FLOOR_P0P1=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPMotionStaleFloor = [regex]::Match($gdbStdout, 'STAGE_MPMOTIONSTALE_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPMotionStaleFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPMOTIONSTALE_FLOOR_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPMotionStaleFloorPos = [regex]::Match($gdbStdout, 'STAGE_MPMOTIONSTALE_FLOOR_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPMotionStaleFloorP0P1 = [regex]::Match($gdbStdout, 'STAGE_MPMOTIONSTALE_FLOOR_P0P1=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffStatusFloor = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFSTATUS_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffStatusFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFSTATUS_FLOOR_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffStatusFloorStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFSTATUS_FLOOR_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffStatusFloorP0P1 = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFSTATUS_FLOOR_P0P1=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPCliffTickFloor = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFTICK_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffTickFloorCalls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFTICK_FLOOR_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffTickFloorStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFTICK_FLOOR_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPFallMapFloor = [regex]::Match($gdbStdout, 'STAGE_MPFALLMAP_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPFallMapFloorCalls = [regex]::Match($gdbStdout, 'STAGE_MPFALLMAP_FLOOR_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPFallMapFloorStatus = [regex]::Match($gdbStdout, 'STAGE_MPFALLMAP_FLOOR_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPFallLandFloor = [regex]::Match($gdbStdout, 'STAGE_MPFALLLAND_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPFallLandFloorCalls = [regex]::Match($gdbStdout, 'STAGE_MPFALLLAND_FLOOR_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPFallLandFloorStatus = [regex]::Match($gdbStdout, 'STAGE_MPFALLLAND_FLOOR_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPCeilFloor = [regex]::Match($gdbStdout, 'STAGE_MPCEIL_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCeilFloorSetup = [regex]::Match($gdbStdout, 'STAGE_MPCEIL_FLOOR_SETUP=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPCeilFloorCheck = [regex]::Match($gdbStdout, 'STAGE_MPCEIL_FLOOR_CHECK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCeilFloorQuery = [regex]::Match($gdbStdout, 'STAGE_MPCEIL_FLOOR_QUERY=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCeilFloorPos = [regex]::Match($gdbStdout, 'STAGE_MPCEIL_FLOOR_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPCeilStatusFloor = [regex]::Match($gdbStdout, 'STAGE_MPCEILSTATUS_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCeilStatusFloorCalls = [regex]::Match($gdbStdout, 'STAGE_MPCEILSTATUS_FLOOR_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCeilStatusFloorStatus = [regex]::Match($gdbStdout, 'STAGE_MPCEILSTATUS_FLOOR_STATUS=(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCeilStatusFloorPos = [regex]::Match($gdbStdout, 'STAGE_MPCEILSTATUS_FLOOR_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageMPCliffCatchFloor = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCATCH_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffCatchFloorCalls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCATCH_FLOOR_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffCatchFloorEffects = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCATCH_FLOOR_EFFECTS=([0-9]+),([0-9]+)')
    $stageMPCliffCatchFloorStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCATCH_FLOOR_STATUS=(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPCliffCatchFloorPos = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCATCH_FLOOR_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageMPCliffCatchFloorOcc = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCATCH_FLOOR_OCC=([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffWaitFloor = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffWaitFloorCalls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_FLOOR_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffWaitFloorStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_FLOOR_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPCliffAttackFloor = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFATTACK_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffAttackFloorCalls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFATTACK_FLOOR_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffAttackFloorStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFATTACK_FLOOR_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPCliffAttackFloorSafe = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFATTACK_FLOOR_SAFE=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+)')
    $stageMPCliffClimbFloor = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FLOOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffClimbFloorCalls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FLOOR_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffClimbFloorBase = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FLOOR_BASE=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPCliffClimbFloorClimb = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FLOOR_CLIMB=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPCliffClimbFloorDrop = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FLOOR_DROP=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffClimbFloorRecatch = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FLOOR_RECATCH=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffClimbFloorRecatchStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FLOOR_RECATCH_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageMPCliffClimbAction = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_ACTION=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffClimbActionCalls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_ACTION_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffClimbActionStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_ACTION_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPCliffClimbActionFlags = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_ACTION_FLAGS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffClimbActionRoot = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_ACTION_ROOT=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPCliffClimbCommon2 = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_COMMON2=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffClimbCommon2Calls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_COMMON2_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffClimbCommon2Status = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_COMMON2_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffClimbCommon2Flags = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_COMMON2_FLAGS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffClimbFinish = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FINISH=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffClimbFinishCalls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FINISH_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffClimbFinishStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FINISH_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffClimbFinishFlags = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCLIMB_FINISH_FLAGS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageMPCliffWaitDamage = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffWaitDamageCalls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffWaitDamageStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffWaitDamageFlags = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_FLAGS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPCliffWaitDamagePos = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_POS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPCliffWaitDamageTick = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_TICK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffWaitDamageCollision = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_COLLISION=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffWaitDamageCliffCatch = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_CLIFFCATCH=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageMPCliffWaitDamagePassiveStand = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPCliffWaitDamagePassive = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffWaitDamagePassiveStandCallbacks = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND_CALLBACKS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffWaitDamagePassiveCallbacks = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE_CALLBACKS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffWaitDamagePassiveStandTick = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND_TICK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPCliffWaitDamagePassiveTick = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE_TICK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPCliffWaitDamageDownBounce = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_DOWNBOUNCE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPCliffWaitDamageDownWait = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_DOWNWAIT=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPCliffWaitDamageDownStand = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFWAIT_DAMAGE_DOWNSTAND=([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPassiveLoop = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPPassiveLoopBranch = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_BRANCH=(0x[0-9a-fA-F]+|0)')
    $stageMPPassiveLoopPassiveStandB = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_PASSIVESTANDB=(0x[0-9a-fA-F]+|0)')
    $stageMPPassiveLoopNaturalMap = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_NATURALMAP=(0x[0-9a-fA-F]+|0)')
    $stageMPPassiveLoopNaturalMapCalls = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_NATURALMAP_CALLS=([0-9]+)')
    $stageMPPassiveLoopSetup = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopPassiveStand = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_PASSIVESTAND=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPPassiveLoopPassive = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_PASSIVE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPPassiveLoopFinal = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_FINAL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopAppeal = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_APPEAL=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopAppealGuard = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_APPEAL_GUARD=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopCatch = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_CATCH=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopCatchCallbacks = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_CATCH_CALLBACKS=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopCatchPull = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_CATCH_PULL=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopCapture = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_CAPTURE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPPassiveLoopThrow = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_THROW=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopThrowB = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_THROW_B=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopThrowCallback = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_THROW_CALLBACK=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopThrowUpdate = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_THROW_UPDATE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPassiveLoopThrowRelease = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_THROW_RELEASE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPassiveLoopThrowReleaseStatus = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_THROW_RELEASE_STATUS=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPPassiveLoopThrowProcStatus = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_THROW_PROC_STATUS=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPPassiveLoopThrowDeadResult = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_THROW_DEAD_RESULT=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopWallDamage = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_WALLDAMAGE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopWallDamageState = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_WALLDAMAGE_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopWallDamageVec = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_WALLDAMAGE_VEC=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPPassiveLoopRebound = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_REBOUND=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPPassiveLoopReboundState = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_REBOUND_STATE=([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPPassiveLoopReboundVec = [regex]::Match($gdbStdout, 'STAGE_MPPASSIVE_REBOUND_VEC=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPDownWaitLoop = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPDownWaitLoopSetup = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPDownWaitLoopInterrupt = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_INTERRUPT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPDownWaitLoopInput = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_INPUT=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPDownWaitLoopStatus = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_STATUS=([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPDownWaitLoopDownStandTick = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_DOWNSTAND_TICK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPDownWaitLoopAttack = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_ATTACK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPDownWaitLoopAttackTick = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_ATTACK_TICK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPDownWaitLoopRoll = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_ROLL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageMPDownWaitLoopRollStatus = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_ROLL_STATUS=(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPDownWaitLoopRollForwardTick = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_ROLL_FORWARD_TICK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPDownWaitLoopRollBackTick = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_ROLL_BACK_TICK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPDownWaitLoopRollMove = [regex]::Match($gdbStdout, 'STAGE_MPDOWNWAIT_ROLL_MOVE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageTurnLoop = [regex]::Match($gdbStdout, 'STAGE_TURN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageTurnLoopCalls = [regex]::Match($gdbStdout, 'STAGE_TURN_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageTurnLoopInput = [regex]::Match($gdbStdout, 'STAGE_TURN_INPUT=(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageTurnLoopSetup = [regex]::Match($gdbStdout, 'STAGE_TURN_SETUP=([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageTurnLoopUpdate = [regex]::Match($gdbStdout, 'STAGE_TURN_UPDATE=([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageTurnLoopFinal = [regex]::Match($gdbStdout, 'STAGE_TURN_FINAL=([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPDownRecoverLoop = [regex]::Match($gdbStdout, 'STAGE_MPDOWNRECOVER=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPDownRecoverLoopSetup = [regex]::Match($gdbStdout, 'STAGE_MPDOWNRECOVER_SETUP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPDownRecoverLoopDownStand = [regex]::Match($gdbStdout, 'STAGE_MPDOWNRECOVER_DOWNSTAND=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPDownRecoverLoopAttack = [regex]::Match($gdbStdout, 'STAGE_MPDOWNRECOVER_ATTACK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $stageMPDownRecoverLoopRoll = [regex]::Match($gdbStdout, 'STAGE_MPDOWNRECOVER_ROLL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPDownRecoverLoopFinal = [regex]::Match($gdbStdout, 'STAGE_MPDOWNRECOVER_FINAL=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffLedgeLoop = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFLEDGE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffLedgeLoopBase = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFLEDGE_BASE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPCliffLedgeLoopState = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFLEDGE_STATE=([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffLedgeLoopLines = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFLEDGE_LINES=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPCliffLiveLoop = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFLIVE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffLiveLoopBase = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFLIVE_BASE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $stageMPCliffLiveLoopCounts = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFLIVE_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffLiveLoopStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFLIVE_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPCliffLiveLoopFinal = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFLIVE_FINAL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffLiveLoopDrop = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFLIVE_DROP=(-?[0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffAttackAction = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFATTACK_ACTION=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffAttackActionCalls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFATTACK_ACTION_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffAttackActionStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFATTACK_ACTION_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPCliffAttackActionFlags = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFATTACK_ACTION_FLAGS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffAttackActionRoot = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFATTACK_ACTION_ROOT=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPCliffCommon2 = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCOMMON2=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffCommon2Calls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCOMMON2_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffCommon2Status = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCOMMON2_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffCommon2Flags = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFCOMMON2_FLAGS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffEscapeAction = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFESCAPE_ACTION=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffEscapeActionCalls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFESCAPE_ACTION_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffEscapeActionStatus = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFESCAPE_ACTION_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffEscapeActionLedge = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFESCAPE_ACTION_LEDGE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(-?[0-9]+)')
    $stageMPCliffEscapeActionFlags = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFESCAPE_ACTION_FLAGS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageMPCliffEscapeActionRoot = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFESCAPE_ACTION_ROOT=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $stageMPCliffEscapeCommon2 = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFESCAPE_COMMON2=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $stageMPCliffEscapeCommon2Calls = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFESCAPE_COMMON2_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffEscapeCommon2Status = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFESCAPE_COMMON2_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $stageMPCliffEscapeCommon2Flags = [regex]::Match($gdbStdout, 'STAGE_MPCLIFFESCAPE_COMMON2_FLAGS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq $ExpectedMode -and [int]$harn.Groups[3].Value -eq $ExpectedHarnessSceneCurr -and [int]$harn.Groups[4].Value -eq $ExpectedHarnessScenePrev -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) $HarnessSelectMessage $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene is not Pupupu VSBattle from Maps.' $gdbStdout
    if (($ExpectedMode -ge 58) -and (($ExpectedMode % 2) -eq 0)) {
        Assert-Condition ($vs.Success -and (Convert-MarkerUInt32 $vs.Groups[1].Value) -eq 0x56535452 -and ((Convert-MarkerUInt32 $vs.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain VS Mode -> PlayersVS transition did not pass.' $gdbStdout
        Assert-Condition ($pv.Success -and (Convert-MarkerUInt32 $pv.Groups[1].Value) -eq 0x50565452 -and ((Convert-MarkerUInt32 $pv.Groups[2].Value) -band 0xff) -eq 0xff) 'Menu-chain PlayersVS -> Maps transition did not pass.' $gdbStdout
        Assert-Condition ($maps.Success -and (Convert-MarkerUInt32 $maps.Groups[1].Value) -eq 0x4d53454c -and ((Convert-MarkerUInt32 $maps.Groups[2].Value) -band 0xff) -eq 0xff -and [int]$maps.Groups[3].Value -eq 6) 'Menu-chain Maps -> VSBattle transition did not pass.' $gdbStdout
    }
    $pr = Get-Ints $prev
    Assert-Condition ($prev.Success -and $pr[0] -eq 0x4647414c -and $pr[1] -eq 0x46474153 -and (($pr[2] -band 0x1fff) -eq 0x1fff) -and $pr[3] -eq 0xff -and $pr[4] -eq 2) 'Prerequisite gcRunAll-loop proof did not pass.' $gdbStdout
    $lp = Get-Ints $loop
    Assert-Condition ($loop.Success -and $lp[0] -eq 0x46444150 -and $lp[1] -eq 0x46444153 -and (($lp[2] -band 0x1fff) -eq 0x1fff) -and $lp[3] -eq 0xff -and $lp[4] -eq 2 -and $lp[5] -ge 160 -and $lp[6] -ge 220) 'gcDrawAll-loop result/mask did not pass.' $gdbStdout
    $tm = Get-Ints $taskman
    Assert-Condition ($tm[0] -eq 1 -and $tm[1] -gt 0 -and $tm[2] -gt 0 -and $tm[3] -eq $tm[2] -and $tm[4] -gt 0 -and $tm[5] -gt 0 -and $tm[6] -gt 0 -and $tm[7] -ge $tm[1]) 'gcDrawAll-loop taskman/draw path failed.' $gdbStdout
    $rn = Get-Ints $run
    Assert-Condition ($rn[0] -gt 0 -and $rn[1] -gt 0 -and $rn[3] -ge 2 -and $rn[4] -eq $rn[5]) 'gcDrawAll pause/preserve/GObj stability setup failed.' $gdbStdout
    $pc = Get-Ints $process
    Assert-Condition ($pc[0] -eq 1 -and $pc[1] -eq 1 -and $pc[2] -gt 0 -and $pc[3] -gt 0 -and $pc[4] -eq $pc[2] -and $pc[5] -eq $pc[3] -and $pc[6] -eq 0) 'gcDrawAll process attach/callback path failed.' $gdbStdout
    $inp = Get-Ints $input
    Assert-Condition ($inp[0] -gt 0 -and $inp[1] -gt 0 -and $inp[2] -gt 0 -and $inp[3] -gt 0 -and $inp[4] -eq 0 -and $inp[5] -eq 0 -and $inp[6] -gt 0 -and $inp[7] -gt 0 -and $inp[8] -ne 0 -and $inp[9] -ne 0 -and $inp[10] -ne 0 -and $inp[11] -ne 0 -and $inp[12] -gt 0 -and $inp[13] -gt 0) 'gcDrawAll input bridge failed or wrote FTStruct directly.' $gdbStdout
    $st = Get-Ints $status
    Assert-Condition (($st[4] -eq 10 -and $st[5] -eq 10 -and $st[6] -eq 4 -and $st[7] -eq 4 -and $st[8] -eq 0 -and $st[9] -eq 0) -and (($st[10] -band 0x3ff) -eq 0x3ff) -and (($st[11] -band 0x3ff) -eq 0x3ff) -and (($st[12] -band 0x7ff) -eq 0x7ff) -and (($st[13] -band 0x7ff) -eq 0x7ff)) 'gcDrawAll status/transition masks were incomplete.' $gdbStdout
    $dr = Get-Ints $draw
    Assert-Condition ($dr[0] -eq 96 -and $dr[1] -eq 72 -and $dr[2] -ge 96 -and $dr[3] -eq 1 -and $dr[4] -ge 7 -and $dr[5] -ge 7 -and $dr[6] -ge 14 -and $dr[7] -gt 0 -and $dr[8] -gt 0 -and $dr[9] -ge 14 -and $dr[10] -ge 18 -and $dr[11] -ge 14 -and $dr[12] -ge 18 -and $dr[13] -gt 0 -and $dr[14] -ne 0 -and $dr[15] -ne 0 -and $dr[16] -ge $dr[6] -and $dr[17] -eq 0) 'gcDrawAll draw/camera/display/DObj/pixel markers failed.' $gdbStdout
    $sc = Get-Ints $screen
    Assert-Condition ($sc[4] -ne 0 -and $sc[5] -ne 0 -and $sc[8] -gt 0 -and $sc[9] -gt 0) 'gcDrawAll screen movement markers failed.' $gdbStdout
    $mv = Get-Ints $move
    $motionStaleP0RootYOK = $false
    if ($RequireStageMPMotionStaleFloor) {
        $mlposForMove = Get-Ints $stageMPMotionStaleFloorPos
        $motionStaleP0RootYOK = $stageMPMotionStaleFloorPos.Success -and
            ([Math]::Abs($mv[6] - $mlposForMove[3]) -le 1)
    }
    $platformP0RootYOK = $RequireStageMPPlatformFloor -and
        $stageMPPlatformFloor.Success -and
        $stageMPPlatformFloorBase.Success -and
        $stageMPPlatformFloorLine.Success -and
        $stageMPPlatformFloorDObj.Success -and
        ($mv[6] -ne 0) -and
        ($mv[10] -eq 1)
    $floorFollowP0RootYOK = $false
    $floorFollowP1RootYOK = $false
    if ($RequireStageFloorFollow) {
        $floorFollowP0ForMove = Get-Ints $stageFloorFollowP0
        $floorFollowP1ForMove = Get-Ints $stageFloorFollowP1
        $floorFollowP0RootYOK = $stageFloorFollowP0.Success -and
            ([Math]::Abs($mv[6] - $floorFollowP0ForMove[6]) -le 1) -and
            ($floorFollowP0ForMove[8] -eq 1)
        $floorFollowP1RootYOK = $stageFloorFollowP1.Success -and
            ([Math]::Abs($mv[7] - $floorFollowP1ForMove[6]) -le 1) -and
            ($floorFollowP1ForMove[8] -eq 1)
    }
    $p0RootYOK = (($mv[6] -eq $mv[0]) -or $motionStaleP0RootYOK -or $platformP0RootYOK -or $floorFollowP0RootYOK)
    $p1RootYOK = (($mv[7] -eq $mv[1]) -or $floorFollowP1RootYOK)
    Assert-Condition ($mv[2] -ne 0 -and $mv[3] -ne 0 -and $mv[4] -gt 0 -and $mv[5] -gt 0 -and $p0RootYOK -and $p1RootYOK -and $mv[8] -eq 1 -and $mv[9] -eq 1 -and $mv[10] -eq 1 -and $mv[11] -eq 1) 'gcDrawAll movement/floor markers failed.' $gdbStdout
    $tr = Get-Ints $trans
    Assert-Condition ($tr[0] -ge 2 -and $tr[1] -ge 2 -and $tr[2] -ge 2 -and $tr[3] -ge 2 -and $tr[4] -ge 4 -and $tr[5] -ge 2 -and $tr[6] -ge 2 -and $tr[7] -ge 2 -and $tr[8] -gt 0) 'gcDrawAll transition counters failed.' $gdbStdout
    $sf = Get-Ints $safe
    Assert-Condition ((@($sf[0..9] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'gcDrawAll safe escape counters were not zero.' $gdbStdout
    $pd = Get-Ints $platform
    Assert-Condition ($pd[0] -eq 1 -and $pd[1] -eq 96 -and $pd[2] -eq 72 -and $pd[3] -ge $dr[4]) 'Platform original-DL preview markers failed.' $gdbStdout
    $stageSummary = ''
    $dashRunSummary = ''
    $mpDamageRecoverSummary = ''
    $mpLiveHitSummary = ''
    if ($RequireStageMPPassiveRecoverLoop) {
        $dashRunInts = Get-Ints $dashRun
        Assert-Condition ($dashRun.Success -and $dashRunInts[0] -eq 0x4644524e -and $dashRunInts[1] -eq 0x46445253 -and (($dashRunInts[2] -band 0x3fffffff) -eq 0x3fffffff) -and $dashRunInts[3] -eq 0xff -and $dashRunInts[4] -eq 2) 'Dash-Run attack/guard aggregate proof did not pass in the current Passive recover boundary.' $gdbStdout
        $dashRunAttackEventPosInts = Get-Ints $dashRunAttackEventPos
        Assert-Condition ($dashRunAttackEventPos.Success -and (($dashRunAttackEventPosInts[0] -band 0x1ffff) -eq 0x1ffff) -and $dashRunAttackEventPosInts[1] -eq 3 -and $dashRunAttackEventPosInts[2] -eq 1 -and $dashRunAttackEventPosInts[3] -eq 14 -and $dashRunAttackEventPosInts[7] -eq 0 -and $dashRunAttackEventPosInts[8] -eq 0 -and (($dashRunAttackEventPosInts[9] -band 0x3) -eq 0x3)) 'Dash-Run selected hitbox position/range/rectangle/collide/record/hit-log/SFX/stats/proc-params proof did not pass in the current Passive recover boundary.' $gdbStdout
        $dashRunProcParamsInts = Get-Ints $dashRunProcParams
        $dashRunProcParamsRequiredMask = Convert-MarkerUInt32 '0xfffdf3ff'
        Assert-Condition ($dashRunProcParams.Success -and (($dashRunProcParamsInts[0] -band $dashRunProcParamsRequiredMask) -eq $dashRunProcParamsRequiredMask) -and ($dashRunProcParamsInts[2] -eq ($dashRunProcParamsInts[1] + $dashRunProcParamsInts[3])) -and $dashRunProcParamsInts[3] -gt 0 -and $dashRunProcParamsInts[4] -gt 0 -and $dashRunProcParamsInts[5] -gt 0 -and $dashRunProcParamsInts[6] -eq 1 -and $dashRunProcParamsInts[7] -eq $dashRunProcParamsInts[8]) 'Dash-Run ftMainProcParams damage/hitlag scheduling proof did not pass in the current Passive recover boundary.' $gdbStdout
        $dashRunProcParamsRumbleInts = Get-Ints $dashRunProcParamsRumble
        Assert-Condition ($dashRunProcParamsRumble.Success -and (($dashRunProcParamsRumbleInts[0] -band 0x7f) -eq 0x7f) -and $dashRunProcParamsRumbleInts[1] -ge 2) 'Dash-Run ftMainProcParams attack-damage/BatSwing4 rumble and attack-rebound promotion proof did not pass.' $gdbStdout
        $dashRunDamageStatusInts = Get-Ints $dashRunDamageStatus
        Assert-Condition ($dashRunDamageStatus.Success -and (($dashRunDamageStatusInts[0] -band 0x1f) -eq 0x1f) -and $dashRunDamageStatusInts[1] -ge 0 -and $dashRunDamageStatusInts[1] -le 3 -and $dashRunDamageStatusInts[2] -ge 0 -and $dashRunDamageStatusInts[2] -le 2 -and $dashRunDamageStatusInts[3] -ge 37 -and $dashRunDamageStatusInts[3] -le 56 -and $dashRunDamageStatusInts[4] -ge 46 -and $dashRunDamageStatusInts[4] -le 53 -and (($dashRunDamageStatusInts[5] -eq 49) -or ($dashRunDamageStatusInts[5] -eq 50))) 'Dash-Run ftcommondamage status selection proof did not pass in the current Passive recover boundary.' $gdbStdout
        $dashRunDamageSetupInts = Get-Ints $dashRunDamageSetup
        $dashRunDamageSetupVelocitySource = 0
        if ($dashRunDamageSetup.Success) {
            $dashRunDamageSetupVelocitySource = if ($dashRunDamageSetupInts[4] -eq 1) { $dashRunDamageSetupInts[8] } else { $dashRunDamageSetupInts[7] }
        }
        $dashRunDamageSetupRequiredMask = Convert-MarkerUInt32 '0xbffffdfd'
        Assert-Condition ($dashRunDamageSetup.Success -and (($dashRunDamageSetupInts[0] -band $dashRunDamageSetupRequiredMask) -eq $dashRunDamageSetupRequiredMask) -and $dashRunDamageSetupInts[1] -eq $dashRunProcParamsInts[7] -and $dashRunDamageSetupInts[2] -ge 37 -and $dashRunDamageSetupInts[2] -le 56 -and $dashRunDamageSetupInts[3] -ge 31 -and $dashRunDamageSetupInts[3] -le 49 -and $dashRunDamageSetupInts[4] -ge 0 -and $dashRunDamageSetupInts[4] -le 1 -and $dashRunDamageSetupInts[5] -gt 0 -and $dashRunDamageSetupInts[6] -eq ($dashRunDamageSetupInts[5] - 1) -and (($dashRunDamageSetupInts[7] -ne 0) -or ($dashRunDamageSetupInts[8] -ne 0) -or ($dashRunDamageSetupInts[9] -ne 0)) -and [Math]::Abs($dashRunDamageSetupInts[10]) -lt [Math]::Abs($dashRunDamageSetupVelocitySource)) 'Dash-Run ftcommondamage status setup/update/physics/flyroll/knockback-invincible/lag-update/hitlag-lifecycle/setup-tail/proc-passive dispatch/proc-passive status/air-map/interrupt/expiry/fall-physics/fastfall/map-floor-cliff proof did not pass in the current Passive recover boundary.' $gdbStdout
        $dashRunDamageDustInts = Get-Ints $dashRunDamageDust
        Assert-Condition ($dashRunDamageDust.Success -and (($dashRunDamageDustInts[0] -band 0xff) -eq 0xff) -and $dashRunDamageDustInts[1] -eq 0x123580) 'Dash-Run ftCommonDamageSetDustEffectInterval threshold proof did not pass.' $gdbStdout
        $dashRunDamageDustUpdateInts = Get-Ints $dashRunDamageDustUpdate
        Assert-Condition ($dashRunDamageDustUpdate.Success -and (($dashRunDamageDustUpdateInts[0] -band 0x1f) -eq 0x1f) -and $dashRunDamageDustUpdateInts[1] -eq 1 -and $dashRunDamageDustUpdateInts[2] -eq 5) 'Dash-Run ftCommonDamageUpdateDustEffect runtime proof did not pass.' $gdbStdout
        $dashRunDamageHitstunPublicInts = Get-Ints $dashRunDamageHitstunPublic
        Assert-Condition ($dashRunDamageHitstunPublic.Success -and (($dashRunDamageHitstunPublicInts[0] -band 0xf) -eq 0xf) -and $dashRunDamageHitstunPublicInts[1] -eq 0 -and $dashRunDamageHitstunPublicInts[2] -eq 456000) 'Dash-Run ftCommonDamageDecHitStunSetPublic proof did not pass.' $gdbStdout
        $dashRunDamageColAnimInts = Get-Ints $dashRunDamageColAnim
        Assert-Condition ($dashRunDamageColAnim.Success -and (($dashRunDamageColAnimInts[0] -band 0x3f) -eq 0x3f) -and $dashRunDamageColAnimInts[1] -eq 0x0522020e -and $dashRunDamageColAnimInts[2] -eq 4) 'Dash-Run ftCommonDamageCheckElementSetColAnim proof did not pass.' $gdbStdout
        $dashRunDamageColAnimUpdateInts = Get-Ints $dashRunDamageColAnimUpdate
        Assert-Condition ($dashRunDamageColAnimUpdate.Success -and (($dashRunDamageColAnimUpdateInts[0] -band 0x8) -eq 0x8)) 'Dash-Run ftCommonDamageUpdateDamageColAnim/SetDamageColAnim proof did not pass.' $gdbStdout
        $dashRunDamageInvincibleInts = Get-Ints $dashRunDamageInvincible
        Assert-Condition ($dashRunDamageInvincible.Success -and (($dashRunDamageInvincibleInts[0] -band 0x1f) -eq 0x1f) -and $dashRunDamageInvincibleInts[1] -ge 1 -and $dashRunDamageInvincibleInts[2] -eq 2) 'Dash-Run ftCommonDamageCheckSetInvincible gate proof did not pass.' $gdbStdout
        $dashRunDamageLagUpdateInts = Get-Ints $dashRunDamageLagUpdate
        Assert-Condition ($dashRunDamageLagUpdate.Success -and (($dashRunDamageLagUpdateInts[0] -band 0x3f) -eq 0x3f) -and $dashRunDamageLagUpdateInts[1] -gt 0 -and $dashRunDamageLagUpdateInts[2] -eq 0) 'Dash-Run ftCommonDamageCommonProcLagUpdate gate proof did not pass.' $gdbStdout
        $dashRunDamageCommonPhysicsInts = Get-Ints $dashRunDamageCommonPhysics
        Assert-Condition ($dashRunDamageCommonPhysics.Success -and (($dashRunDamageCommonPhysicsInts[0] -band 0x3f) -eq 0x3f) -and [Math]::Abs($dashRunDamageCommonPhysicsInts[1]) -gt 0 -and [Math]::Abs($dashRunDamageCommonPhysicsInts[1]) -lt 12000 -and [Math]::Abs($dashRunDamageCommonPhysicsInts[2]) -gt 0 -and [Math]::Abs($dashRunDamageCommonPhysicsInts[2]) -lt 12000 -and $dashRunDamageCommonPhysicsInts[3] -lt 0 -and $dashRunDamageCommonPhysicsInts[4] -eq 0) 'Dash-Run ftCommonDamageCommonProcPhysics branch proof did not pass.' $gdbStdout
        $dashRunDamageCommonCallbacksInts = Get-Ints $dashRunDamageCommonCallbacks
        Assert-Condition ($dashRunDamageCommonCallbacks.Success -and (($dashRunDamageCommonCallbacksInts[0] -band 0x3bfd) -eq 0x3bfd)) 'Dash-Run common damage callback proof did not pass.' $gdbStdout
        $dashRunDamageLevelsInts = Get-Ints $dashRunDamageLevels
        Assert-Condition ($dashRunDamageLevels.Success -and (($dashRunDamageLevelsInts[0] -band 0x1f) -eq 0x1f)) 'Dash-Run ftCommonDamageGetDamageLevel threshold proof did not pass.' $gdbStdout
        $dashRunDamageHoldResistInts = Get-Ints $dashRunDamageHoldResist
        Assert-Condition ($dashRunDamageHoldResist.Success -and (($dashRunDamageHoldResistInts[0] -band 0xff) -eq 0xff)) 'Dash-Run ftCommonDamage catch-resist/capture-hold gate proof did not pass.' $gdbStdout
        $dashRunDamageUpdateCatchResistInts = Get-Ints $dashRunDamageUpdateCatchResist
        Assert-Condition ($dashRunDamageUpdateCatchResist.Success -and (($dashRunDamageUpdateCatchResistInts[0] -band 0xf) -eq 0xf)) 'Dash-Run ftCommonDamageUpdateCatchResist branch proof did not pass.' $gdbStdout
        $dashRunDamageAirMapWallInts = Get-Ints $dashRunDamageAirMapWall
        Assert-Condition ($dashRunDamageAirMapWall.Success -and (($dashRunDamageAirMapWallInts[0] -band 0x3f) -eq 0x3f)) 'Dash-Run DamageAir wall-map short-circuit proof did not pass.' $gdbStdout
        $dashRunDamageKnockbackAngleInts = Get-Ints $dashRunDamageKnockbackAngle
        Assert-Condition ($dashRunDamageKnockbackAngle.Success -and (($dashRunDamageKnockbackAngleInts[0] -band 0x3f) -eq 0x3f)) 'Dash-Run ftCommonDamageGetKnockbackAngle Sakurai-angle proof did not pass.' $gdbStdout
        $dashRunDamageFallInterruptInts = Get-Ints $dashRunDamageFallInterrupt
        Assert-Condition ($dashRunDamageFallInterrupt.Success -and (($dashRunDamageFallInterruptInts[0] -band 0x3f) -eq 0x3f)) 'Dash-Run DamageFall source-order interrupt proof did not pass.' $gdbStdout
        $dashRunDamageScreenFlashInts = Get-Ints $dashRunDamageScreenFlash
        Assert-Condition ($dashRunDamageScreenFlash.Success -and (($dashRunDamageScreenFlashInts[0] -band 0x7f) -eq 0x7f) -and $dashRunDamageScreenFlashInts[1] -eq 0x3a3d3c3b -and $dashRunDamageScreenFlashInts[2] -eq 4) 'Dash-Run ftCommonDamageCheckMakeScreenFlash proof did not pass.' $gdbStdout
        $dashRunDamagePublicInts = Get-Ints $dashRunDamagePublic
        Assert-Condition ($dashRunDamagePublic.Success -and (($dashRunDamagePublicInts[0] -band 0x3f) -eq 0x3f) -and $dashRunDamagePublicInts[1] -eq 160000 -and $dashRunDamagePublicInts[2] -eq 1) 'Dash-Run ftCommonDamageSetPublic public reaction proof did not pass.' $gdbStdout
        $dashRunDamageVoiceInts = Get-Ints $dashRunDamageVoice
        Assert-Condition ($dashRunDamageVoice.Success -and (($dashRunDamageVoiceInts[0] -band 0xf) -eq 0xf) -and $dashRunDamageVoiceInts[1] -ge 2 -and $dashRunDamageVoiceInts[2] -ne $dashRunDamageVoiceInts[3]) 'Dash-Run ftCommonDamageInitDamageVars damage-voice branch proof did not pass.' $gdbStdout
        $dashRunDamageFlyTopInts = Get-Ints $dashRunDamageFlyTop
        Assert-Condition ($dashRunDamageFlyTop.Success -and (($dashRunDamageFlyTopInts[0] -band 0xf) -eq 0xf) -and $dashRunDamageFlyTopInts[1] -eq 54 -and $dashRunDamageFlyTopInts[2] -eq 47 -and $dashRunDamageFlyTopInts[3] -eq 90) 'Dash-Run ftCommonDamageInitDamageVars FlyTop angle branch proof did not pass.' $gdbStdout
        $dashRunDamageReplaceElectricInts = Get-Ints $dashRunDamageReplaceElectric
        Assert-Condition ($dashRunDamageReplaceElectric.Success -and (($dashRunDamageReplaceElectricInts[0] -band 0x3f) -eq 0x3f) -and $dashRunDamageReplaceElectricInts[1] -eq 50 -and $dashRunDamageReplaceElectricInts[2] -eq 55 -and $dashRunDamageReplaceElectricInts[3] -eq 43 -and $dashRunDamageReplaceElectricInts[4] -eq 55 -and $dashRunDamageReplaceElectricInts[5] -eq 48) 'Dash-Run ftCommonDamageInitDamageVars replacement/electric wrapper proof did not pass.' $gdbStdout
        $dashRunDamageFlyRollInts = Get-Ints $dashRunDamageFlyRoll
        Assert-Condition ($dashRunDamageFlyRoll.Success -and (($dashRunDamageFlyRollInts[0] -band 0x1f) -eq 0x1f) -and $dashRunDamageFlyRollInts[1] -eq 55 -and $dashRunDamageFlyRollInts[2] -eq 48 -and $dashRunDamageFlyRollInts[3] -ge 100) 'Dash-Run ftCommonDamageInitDamageVars FlyRoll random branch proof did not pass.' $gdbStdout
        $dashRunDamageKirbyCopyInts = Get-Ints $dashRunDamageKirbyCopy
        Assert-Condition ($dashRunDamageKirbyCopy.Success -and (($dashRunDamageKirbyCopyInts[0] -band 0x6) -eq 0x6) -and $dashRunDamageKirbyCopyInts[1] -eq 1 -and $dashRunDamageKirbyCopyInts[2] -eq 8 -and $dashRunDamageKirbyCopyInts[3] -eq 204) 'Dash-Run ftCommonDamageInitDamageVars Kirby copy-loss branch proof did not pass.' $gdbStdout
        $dashRunDamageItemHeavyInts = Get-Ints $dashRunDamageItemHeavy
        Assert-Condition ($dashRunDamageItemHeavy.Success -and (($dashRunDamageItemHeavyInts[0] -band 0x1f) -eq 0x1f)) 'Dash-Run ftCommonDamageUpdateMain heavy-item branch proof did not pass.' $gdbStdout
        $dashRunDamageItemBypassInts = Get-Ints $dashRunDamageItemBypass
        Assert-Condition ($dashRunDamageItemBypass.Success -and (($dashRunDamageItemBypassInts[0] -band 0x1f) -eq 0x1f)) 'Dash-Run ftCommonDamageUpdateMain item-bypass fallthrough proof did not pass.' $gdbStdout
        $dashRunDamageKindInts = Get-Ints $dashRunDamageKind
        Assert-Condition ($dashRunDamageKind.Success -and (($dashRunDamageKindInts[0] -band 0x5f) -eq 0x5f)) 'Dash-Run ftMainProcParams Twister/proc_trap branch plus imported ftCommonDamageInitDamageVars/GotoDamageStatus route and damage-kind preservation proof did not pass.' $gdbStdout
        $dashRunDamageSleepInts = Get-Ints $dashRunDamageSleep
        Assert-Condition ($dashRunDamageSleep.Success -and (($dashRunDamageSleepInts[0] -band 0x6f) -eq 0x6f) -and $dashRunDamageSleepInts[2] -eq 165 -and $dashRunDamageSleepInts[4] -gt 0) 'Dash-Run ftCommonDamageUpdateMain sleep-element status branch proof did not pass.' $gdbStdout
        $dashRunDamageLoseGripInts = Get-Ints $dashRunDamageLoseGrip
        Assert-Condition ($dashRunDamageLoseGrip.Success -and (($dashRunDamageLoseGripInts[0] -band 0x7b) -eq 0x7b) -and $dashRunDamageLoseGripInts[1] -gt 0 -and $dashRunDamageLoseGripInts[2] -gt 0 -and $dashRunDamageLoseGripInts[3] -gt 0 -and $dashRunDamageLoseGripInts[6] -gt 0) 'Dash-Run ftCommonThrownDecideFighterLoseGrip original release/collision/link-clear proof did not pass.' $gdbStdout
        $dashRunSummary = " dashRun=0x$('{0:x}' -f $dashRunInts[2]) hitboxPos=0x$('{0:x}' -f $dashRunAttackEventPosInts[0]) hitboxIds=0x$('{0:x}' -f $dashRunAttackEventPosInts[9]) procRumble=0x$('{0:x}' -f $dashRunProcParamsRumbleInts[0]) procRebound=0x$('{0:x}' -f (($dashRunProcParamsRumbleInts[0] -shr 2) -band 0x1f)) damageStatus=0x$('{0:x}' -f $dashRunDamageStatusInts[0]) damageSetup=0x$('{0:x}' -f $dashRunDamageSetupInts[0]) dust=0x$('{0:x}' -f $dashRunDamageDustInts[0]) dustUpdate=0x$('{0:x}' -f $dashRunDamageDustUpdateInts[0]) hitPublic=0x$('{0:x}' -f $dashRunDamageHitstunPublicInts[0]) colAnim=0x$('{0:x}' -f $dashRunDamageColAnimInts[0]) colAnimUpdate=0x$('{0:x}' -f $dashRunDamageColAnimUpdateInts[0]) invGate=0x$('{0:x}' -f $dashRunDamageInvincibleInts[0]) lagUpdate=0x$('{0:x}' -f $dashRunDamageLagUpdateInts[0]) commonPhys=0x$('{0:x}' -f $dashRunDamageCommonPhysicsInts[0]) commonCb=0x$('{0:x}' -f $dashRunDamageCommonCallbacksInts[0]) level=0x$('{0:x}' -f $dashRunDamageLevelsInts[0]) hold=0x$('{0:x}' -f $dashRunDamageHoldResistInts[0]) catchResist=0x$('{0:x}' -f $dashRunDamageUpdateCatchResistInts[0]) airMapWall=0x$('{0:x}' -f $dashRunDamageAirMapWallInts[0]) angle=0x$('{0:x}' -f $dashRunDamageKnockbackAngleInts[0]) fallInterrupt=0x$('{0:x}' -f $dashRunDamageFallInterruptInts[0]) flash=0x$('{0:x}' -f $dashRunDamageScreenFlashInts[0]) public=0x$('{0:x}' -f $dashRunDamagePublicInts[0]) voice=0x$('{0:x}' -f $dashRunDamageVoiceInts[0]) flytop=0x$('{0:x}' -f $dashRunDamageFlyTopInts[0]) replace=0x$('{0:x}' -f $dashRunDamageReplaceElectricInts[0]) flyroll=0x$('{0:x}' -f $dashRunDamageFlyRollInts[0]) kirbycopy=0x$('{0:x}' -f $dashRunDamageKirbyCopyInts[0]) itemHeavy=0x$('{0:x}' -f $dashRunDamageItemHeavyInts[0]) itemBypass=0x$('{0:x}' -f $dashRunDamageItemBypassInts[0]) dmgKind=0x$('{0:x}' -f $dashRunDamageKindInts[0]) sleep=0x$('{0:x}' -f $dashRunDamageSleepInts[0]) loseGrip=0x$('{0:x}' -f $dashRunDamageLoseGripInts[0])/$($dashRunDamageLoseGripInts[1])/$($dashRunDamageLoseGripInts[2])"
    }
    if ($RequireStageMPDamageRecoverLoop) {
        $md = Get-Ints $stageMPDamageRecover
        Assert-Condition ($stageMPDamageRecover.Success -and $md[0] -eq 0x46445243 -and $md[1] -eq 0x46445253 -and (($md[2] -band 0xffff) -eq 0xffff) -and $md[3] -eq 0xff -and $md[4] -eq 1) 'Stage MP damage-recover source-chain result/mask did not pass.' $gdbStdout
        $mds = Get-Ints $stageMPDamageRecoverSetup
        Assert-Condition ($stageMPDamageRecoverSetup.Success -and $mds[0] -eq 1 -and $mds[1] -eq 1 -and $mds[2] -eq 1 -and $mds[3] -gt 0 -and $mds[4] -gt 0 -and $mds[5] -gt 0 -and $mds[6] -gt 0 -and $mds[7] -gt 0) 'Stage MP damage-recover setup/base/contact markers failed.' $gdbStdout
        $mdc = Get-Ints $stageMPDamageRecoverContact
        Assert-Condition ($stageMPDamageRecoverContact.Success -and $mdc[0] -ge 0 -and $mdc[1] -ge 0 -and $mdc[0] -ne $mdc[1] -and $mdc[2] -gt 0 -and $mdc[3] -gt 0 -and $mdc[4] -gt 0 -and $mdc[5] -gt 0 -and $mdc[6] -gt 0 -and $mdc[7] -gt 0 -and $mdc[8] -gt 0 -and $mdc[9] -gt 0) 'Stage MP damage-recover selected contact/proc-lag markers failed.' $gdbStdout
        $mdd = Get-Ints $stageMPDamageRecoverDamage
        Assert-Condition ($stageMPDamageRecoverDamage.Success -and $mdd[1] -gt $mdd[0] -and (($mdd[3] -gt $mdd[2]) -or ($mdd[3] -eq ($mdd[1] - $mdd[0]))) -and $mdd[4] -gt 0 -and $mdd[5] -gt $mdd[4] -and $mdd[6] -ne 0) 'Stage MP damage-recover damage queue/hitlag/knockback markers failed.' $gdbStdout
        $mdst = Get-Ints $stageMPDamageRecoverStatus
        Assert-Condition ($stageMPDamageRecoverStatus.Success -and $mdst[1] -eq $mdst[0] -and $mdst[2] -ge 31 -and $mdst[4] -eq $mdst[3] -and $mdst[6] -ge 37) 'Stage MP damage-recover damage-status markers failed.' $gdbStdout
        $mdcb = Get-Ints $stageMPDamageRecoverCallbacks
        Assert-Condition ($stageMPDamageRecoverCallbacks.Success -and (@($mdcb[0..9] | Where-Object { $_ -le 0 }).Count -eq 0)) 'Stage MP damage-recover callback markers failed.' $gdbStdout
        $mdg = Get-Ints $stageMPDamageRecoverGround
        Assert-Condition ($stageMPDamageRecoverGround.Success -and $mdg[0] -gt 0 -and $mdg[1] -gt 0 -and $mdg[2] -gt 0 -and $mdg[3] -gt 0 -and $mdg[4] -gt 0 -and $mdg[5] -gt 0 -and $mdg[6] -ge 37) 'Stage MP damage-recover ground/DamageFall markers failed.' $gdbStdout
        $mdb = Get-Ints $stageMPDamageRecoverBranches
        Assert-Condition ($stageMPDamageRecoverBranches.Success -and (@($mdb[0..8] | Where-Object { $_ -le 0 }).Count -eq 0)) 'Stage MP damage-recover PassiveStand/Passive/DownBounce branch markers failed.' $gdbStdout
        $mdv = Get-Ints $stageMPDamageRecoverVel
        Assert-Condition ($stageMPDamageRecoverVel.Success -and (($mdv[0] -ne 0) -or ($mdv[1] -ne 0) -or ($mdv[2] -ne 0))) 'Stage MP damage-recover velocity markers failed.' $gdbStdout
        $mdsafe = Get-Ints $stageMPDamageRecoverSafe
        Assert-Condition ($stageMPDamageRecoverSafe.Success -and $mdsafe[0] -ge 0 -and $mdsafe[1] -ge 0 -and $mdsafe[2] -eq 1 -and $mdsafe[3] -eq 1 -and $mdsafe[4] -eq 0 -and $mdsafe[5] -eq 0 -and $mdsafe[6] -eq 0) 'Stage MP damage-recover safety/floor markers failed.' $gdbStdout
        $mpDamageRecoverSummary = " mpDamageRecover=contact=$($mds[4])/$($mds[5]) dmg=$($mdd[0])->$($mdd[1]) hitlag=$($mdd[4]) status=$($mdst[1])/$($mdst[2]) fall=$($mdst[6])/$($mdst[7]) ps=$($mdb[1]) passive=$($mdb[4]) dbounce=$($mdb[7])"
    }
    if ($RequireStageMPLiveHitDamageLoop) {
        $ml = Get-Ints $stageMPLiveHitDamage
        $stageMPLiveHitDamageExpectedMask = [Convert]::ToUInt32('ffffffff', 16)
        Assert-Condition ($stageMPLiveHitDamage.Success -and $ml[0] -eq 0x464c4844 -and $ml[1] -eq 0x464c4853 -and (($ml[2] -band $stageMPLiveHitDamageExpectedMask) -eq $stageMPLiveHitDamageExpectedMask) -and $ml[3] -eq 0x1 -and $ml[4] -eq 1) 'Stage MP live-hit damage-loop result/mask did not pass.' $gdbStdout
        $mls = Get-Ints $stageMPLiveHitSetup
        Assert-Condition ($stageMPLiveHitSetup.Success -and $mls[0] -eq 1 -and $mls[1] -eq 1 -and $mls[2] -eq 1 -and $mls[3] -ne $mls[4] -and $mls[5] -gt 0 -and $mls[6] -gt 0 -and $mls[7] -eq 1) 'Stage MP live-hit setup/base/state-save markers failed.' $gdbStdout
        $mla = Get-Ints $stageMPLiveHitAttack
        Assert-Condition ($stageMPLiveHitAttack.Success -and $mla[0] -eq 1 -and $mla[1] -eq 191 -and $mla[2] -eq 166 -and $mla[3] -eq 0 -and (($mla[4] -band 0xff) -eq 0xff)) 'Stage MP live-hit Attack12 status/callback markers failed.' $gdbStdout
        $mle = Get-Ints $stageMPLiveHitEvents
        Assert-Condition ($stageMPLiveHitEvents.Success -and $mle[0] -gt 0 -and $mle[1] -gt 0 -and $mle[2] -gt 0 -and $mle[3] -ne 0 -and (($mle[4] -band 0xf) -eq 0xf)) 'Stage MP live-hit animation-event make-attack markers failed.' $gdbStdout
        $mld = Get-Ints $stageMPLiveHitAttackData
        Assert-Condition ($stageMPLiveHitAttackData.Success -and $mld[0] -eq 1 -and $mld[2] -eq 14 -and $mld[3] -gt 0 -and $mld[4] -gt 0 -and $mld[5] -gt 0 -and $mld[7] -gt 0 -and $mld[8] -ne 0) 'Stage MP live-hit selected hitbox metadata failed.' $gdbStdout
        $mlsec = Get-Ints $stageMPLiveHitSecondary
        Assert-Condition ($stageMPLiveHitSecondary.Success -and (($mlsec[0] -band 0x7f) -eq 0x7f) -and $mlsec[1] -eq 0 -and $mlsec[2] -eq 14 -and $mlsec[3] -eq 4 -and $mlsec[4] -eq 100 -and $mlsec[5] -eq 140 -and $mlsec[6] -eq 70 -and (($mlsec[7] -band 0x7) -eq 0x7)) 'Stage MP live-hit secondary Fox Jab2 hitbox decode/contact markers failed.' $gdbStdout
        $mlhb = Get-Ints $stageMPLiveHitHurtbox
        Assert-Condition ($stageMPLiveHitHurtbox.Success -and (($mlhb[0] -band 0x1ffff) -eq 0x1ffff) -and $mlhb[1] -ge 4 -and $mlhb[2] -eq $mlhb[1] -and $mlhb[3] -eq 2 -and $mlhb[4] -eq 2 -and $mlhb[5] -eq 1 -and $mlhb[6] -eq 3 -and $mlhb[7] -gt 0 -and $mlhb[8] -eq 0) 'Stage MP live-hit source-order multi-slot hurtbox scan markers failed.' $gdbStdout
        $mlhbd = Get-Ints $stageMPLiveHitHurtboxDamage
        Assert-Condition ($stageMPLiveHitHurtboxDamage.Success -and (($mlhbd[0] -band 0x7ffff) -eq 0x7ffff) -and $mlhbd[1] -eq $mlhb[6] -and $mlhbd[2] -eq $mlhb[7] -and $mlhbd[4] -gt $mlhbd[3] -and $mlhbd[6] -eq ($mlhbd[5] + $mlhbd[4]) -and $mlhbd[7] -gt 0) 'Stage MP live-hit hurtbox-slot damage-consume markers failed.' $gdbStdout
        $mleff = Get-Ints $stageMPLiveHitEffectOnly
        Assert-Condition ($stageMPLiveHitEffectOnly.Success -and (($mleff[0] -band 0x1ff) -eq 0x1ff) -and $mleff[1] -eq 2 -and $mleff[2] -eq $mld[3] -and $mleff[4] -eq $mleff[3] -and $mleff[6] -eq $mleff[5] -and $mleff[8] -eq $mleff[7] -and $mleff[9] -eq 1 -and $mleff[10] -eq 1 -and $mleff[11] -eq $mleff[2]) 'Stage MP live-hit effect-only/no-damage markers failed.' $gdbStdout
        $mlres = Get-Ints $stageMPLiveHitResist
        Assert-Condition ($stageMPLiveHitResist.Success -and (($mlres[0] -band 0xfff) -eq 0xfff) -and $mlres[1] -eq $mld[3] -and $mlres[2] -gt $mlres[1] -and $mlres[3] -eq ($mlres[2] - $mlres[1]) -and $mlres[4] -eq 1 -and $mlres[6] -eq $mlres[5] -and $mlres[8] -eq $mlres[7] -and $mlres[10] -eq $mlres[9] -and $mlres[11] -eq 1 -and $mlres[12] -eq 1 -and $mlres[13] -eq $mlres[1]) 'Stage MP live-hit damage-resist markers failed.' $gdbStdout
        $mlresb = Get-Ints $stageMPLiveHitResistBreak
        Assert-Condition ($stageMPLiveHitResistBreak.Success -and (($mlresb[0] -band 0x7f) -eq 0x7f) -and $mlresb[1] -gt 0 -and $mlresb[1] -lt $mld[3] -and $mlresb[2] -lt 0 -and $mlresb[3] -eq 0 -and $mlresb[4] -eq (-$mlresb[2]) -and $mlresb[5] -eq $mlresb[4] -and $mlresb[6] -eq $mlresb[4]) 'Stage MP live-hit damage-resist breakthrough markers failed.' $gdbStdout
        $mlta = Get-Ints $stageMPLiveHitThrowAttr
        Assert-Condition ($stageMPLiveHitThrowAttr.Success -and (($mlta[0] -band 0x1f) -eq 0x1f) -and $mlta[1] -ne $mlta[3] -and $mlta[2] -ne $mlta[4] -and $mlta[3] -eq $mlta[5] -and $mlta[4] -eq $mlta[6]) 'Stage MP live-hit throw-owner attribution markers failed.' $gdbStdout
        $mlac = Get-Ints $stageMPLiveHitAttackClash
        Assert-Condition ($stageMPLiveHitAttackClash.Success -and (($mlac[0] -band 0x3f) -eq 0x3f) -and $mlac[1] -eq 4 -and $mlac[2] -eq 2 -and $mlac[3] -eq 24 -and $mlac[4] -eq 18 -and $mlac[5] -eq 42880 -and $mlac[6] -eq 33160 -and $mlac[7] -eq -1 -and $mlac[8] -eq 1 -and $mlac[9] -eq 2) 'Stage MP live-hit attack-vs-attack clash markers failed.' $gdbStdout
        $mlcs = Get-Ints $stageMPLiveHitCatchStat
        Assert-Condition ($stageMPLiveHitCatchStat.Success -and (($mlcs[0] -band 0x1f) -eq 0x1f) -and $mlcs[1] -eq 160000 -and $mlcs[2] -gt $mlcs[1] -and $mlcs[3] -eq $mlcs[1] -and $mlcs[4] -eq 1 -and $mlcs[5] -eq 1) 'Stage MP live-hit catch-stat markers failed.' $gdbStdout
        $mlcse = Get-Ints $stageMPLiveHitCatchSearch
        $catchSearchExpectedMask = [Convert]::ToUInt32('ffffffff', 16)
        Assert-Condition ($stageMPLiveHitCatchSearch.Success -and (($mlcse[0] -band $catchSearchExpectedMask) -eq $catchSearchExpectedMask) -and (($mlcse[1] -band 0x3fff) -eq 0x3fff) -and $mlcse[2] -eq $mlhb[6] -and $mlcse[3] -eq $mlhb[7] -and $mlcse[4] -eq $mlcs[1]) 'Stage MP live-hit catch-search markers failed.' $gdbStdout
        $mlp = Get-Ints $stageMPLiveHitPos
        Assert-Condition ($stageMPLiveHitPos.Success -and $mlp[3] -eq 0 -and $mlp[4] -eq 1 -and $mlp[5] -eq 2 -and $mlp[6] -eq 3 -and $mlp[7] -eq 1) 'Stage MP live-hit attack-state interpolation markers failed.' $gdbStdout
        $mlc = Get-Ints $stageMPLiveHitColl
        Assert-Condition ($stageMPLiveHitColl.Success -and (@($mlc[0..9] | Where-Object { $_ -le 0 }).Count -eq 0)) 'Stage MP live-hit range/rectangle/contact/repeat markers failed.' $gdbStdout
        $mlrh = Get-Ints $stageMPLiveHitRehit
        Assert-Condition ($stageMPLiveHitRehit.Success -and (@($mlrh[0..4] | Where-Object { $_ -le 0 }).Count -eq 0) -and $mlrh[5] -eq 3 -and $mlrh[6] -eq 5 -and $mlrh[7] -eq 0 -and $mlrh[8] -eq 1 -and $mlrh[9] -eq 1 -and (($mlrh[10] -band 0x3f) -eq 0x3f)) 'Stage MP live-hit source-shaped hit-interact/refresh markers failed.' $gdbStdout
        $mlsh = Get-Ints $stageMPLiveHitShield
        Assert-Condition ($stageMPLiveHitShield.Success -and $mlsh[0] -eq 1 -and $mlsh[1] -eq $mld[3] -and $mlsh[2] -eq 0 -and $mlsh[3] -eq $mld[3] -and $mlsh[4] -ge $mlsh[3] -and (($mlsh[5] -eq 1) -or ($mlsh[5] -eq -1)) -and $mlsh[6] -ge 0 -and $mlsh[7] -eq 1 -and $mlsh[8] -eq $mld[3] -and $mlsh[9] -eq 155 -and (($mlsh[10] -eq 134) -or ($mlsh[10] -eq -1)) -and $mlsh[11] -gt 0 -and (($mlsh[12] -band 0x3f) -eq 0x3f)) 'Stage MP live-hit source-shaped shield-stat/GuardSetOff markers failed.' $gdbStdout
        $mlsot = Get-Ints $stageMPLiveHitShieldSetOffTick
        Assert-Condition ($stageMPLiveHitShieldSetOffTick.Success -and (($mlsot[0] -band 0x1f) -eq 0x1f) -and $mlsot[1] -eq 155 -and $mlsot[2] -eq 154 -and $mlsot[3] -eq 1000) 'Stage MP live-hit original GuardSetOff update tick markers failed.' $gdbStdout
        $mlshc = Get-Ints $stageMPLiveHitShieldContact
        Assert-Condition ($stageMPLiveHitShieldContact.Success -and (($mlshc[0] -band 0x7fffff) -eq 0x7fffff) -and $mlshc[1] -eq $mld[0] -and $mlshc[2] -eq 1 -and $mlshc[3] -gt 0 -and $mlshc[4] -gt 0 -and $mlshc[5] -gt 0) 'Stage MP live-hit source-order shield-contact markers failed.' $gdbStdout
        $mlor = Get-Ints $stageMPLiveHitOriginalRehit
        Assert-Condition ($stageMPLiveHitOriginalRehit.Success -and $mlor[0] -eq 30 -and $mlor[1] -eq 5 -and $mlor[2] -eq 30 -and $mlor[3] -eq 29 -and $mlor[4] -eq 0 -and (($mlor[5] -band 0x7) -eq 0x7) -and (($mlor[6] -band 0x3) -eq 0x3) -and $mlor[7] -eq 1 -and (($mlor[8] -band 0x3f) -eq 0x3f) -and (($mlor[9] -band 0x3) -eq 0x3)) 'Stage MP live-hit original rehit timer markers failed.' $gdbStdout
        $mlorh = Get-Ints $stageMPLiveHitOriginalRehitHit
        Assert-Condition ($stageMPLiveHitOriginalRehitHit.Success -and $mlorh[0] -eq 1 -and $mlorh[1] -eq 30 -and $mlorh[2] -eq 40000 -and $mlorh[3] -eq 0 -and $mlorh[4] -eq 213 -and $mlorh[5] -eq 188 -and (($mlorh[6] -band 0x7) -eq 0x7) -and $mlorh[7] -eq 35000) 'Stage MP live-hit original rehit ProcHit markers failed.' $gdbStdout
        $mldmg = Get-Ints $stageMPLiveHitDamageState
        Assert-Condition ($stageMPLiveHitDamageState.Success -and $mldmg[1] -gt $mldmg[0] -and $mldmg[2] -gt 0 -and $mldmg[4] -lt $mldmg[3] -and $mldmg[5] -ne 0 -and (($mldmg[6] -ne 0) -or ($mldmg[7] -ne 0))) 'Stage MP live-hit damage/hitlag/knockback markers failed.' $gdbStdout
        $mlproc = Get-Ints $stageMPLiveHitProc
        Assert-Condition ($stageMPLiveHitProc.Success -and (@($mlproc[0..7] | Where-Object { $_ -le 0 }).Count -eq 0)) 'Stage MP live-hit proc/log/SFX/stat markers failed.' $gdbStdout
        $mlr = Get-Ints $stageMPLiveHitRecover
        Assert-Condition ($stageMPLiveHitRecover.Success -and $mlr[0] -eq 1 -and $mlr[1] -ge 37 -and $mlr[2] -ge 31 -and $mlr[3] -gt 0 -and $mlr[4] -gt 0 -and $mlr[5] -gt 0) 'Stage MP live-hit damage-recover consumption markers failed.' $gdbStdout
        $mlsafe = Get-Ints $stageMPLiveHitSafe
        Assert-Condition ($stageMPLiveHitSafe.Success -and $mlsafe[0] -ge 0 -and $mlsafe[1] -ge 0 -and $mlsafe[2] -eq 1 -and $mlsafe[3] -eq 1 -and $mlsafe[4] -eq 0 -and $mlsafe[5] -eq 0 -and $mlsafe[6] -eq 0 -and $mlsafe[7] -eq 0) 'Stage MP live-hit safety/floor markers failed.' $gdbStdout
        $mpLiveHitSummary = " mpLiveHit=atk=$($mla[1])/$($mla[2]) hitbox=$($mld[0])/j$($mld[2]) dmg=$($mld[3]) second=$($mlsec[1])/j$($mlsec[2])/x$($mlsec[5]) hurt=$($mlhb[6])/$($mlhb[1]) hbdmg=$($mlhbd[3])->$($mlhbd[4])/$($mlhbd[7]) eff=0x$('{0:x}' -f $mleff[0])/$($mleff[3])->$($mleff[4]) resist=0x$('{0:x}' -f $mlres[0])/$($mlres[2])->$($mlres[3]) rbreak=0x$('{0:x}' -f $mlresb[0])/$($mlresb[1])->$($mlresb[2])/q$($mlresb[5]) throwAttr=0x$('{0:x}' -f $mlta[0])/$($mlta[1])->$($mlta[3]) clash=0x$('{0:x}' -f $mlac[0])/$($mlac[3])/$($mlac[4]) catchStat=0x$('{0:x}' -f $mlcs[0])/$($mlcs[1]) catchSearch=0x$('{0:x}' -f $mlcse[0])/s$($mlcse[2]) shield=$($mlsh[1])->$($mlsh[3])/$($mlsh[4]) shc=0x$('{0:x}' -f $mlshc[0])/$($mlshc[5]) so=$($mlsh[9])/$($mlsh[10]) soTick=0x$('{0:x}' -f $mlsot[0])/$($mlsot[1])->$($mlsot[2]) contact=$($mlc[5]) repeat=$($mlc[8]) carry=0x$('{0:x}' -f $mle[4]) gate=0x$('{0:x}' -f $mlrh[10]) rehit=$($mlrh[6])->$($mlrh[7]) origRehit=hit$($mlorh[1])/v$($mlorh[2]) $($mlor[2])->$($mlor[3])->$($mlor[4]) clear=$($mlrh[9])/$('{0:x}' -f $mlor[6]) ids=0x$('{0:x}' -f $mlor[9]) dmg=$($mldmg[0])->$($mldmg[1]) hitlag=$($mldmg[2])"
    }
    if ($RequireStageMPLiveHitStatusLoop) {
        $mlst = Get-Ints $stageMPLiveHitStatus
        Assert-Condition ($stageMPLiveHitStatus.Success -and $mlst[0] -eq 0x464c5450 -and $mlst[1] -eq 0x464c5453 -and $mlst[2] -eq 4294967295 -and $mlst[3] -eq 0x1 -and $mlst[4] -eq 1) 'Stage MP live-hit status-loop result/mask did not pass.' $gdbStdout
        $mlsts = Get-Ints $stageMPLiveHitStatusSetup
        Assert-Condition ($stageMPLiveHitStatusSetup.Success -and $mlsts[0] -eq 1 -and $mlsts[1] -ne $mlsts[2] -and $mlsts[3] -eq 1) 'Stage MP live-hit status-loop setup markers failed.' $gdbStdout
        $mlstsrch = Get-Ints $stageMPLiveHitStatusSearch
        Assert-Condition ($stageMPLiveHitStatusSearch.Success -and (($mlstsrch[0] -band 0xf) -eq 0xf) -and (($mlstsrch[1] -band 0x7ffff) -eq 0x7ffff) -and $mlstsrch[2] -gt 0 -and $mlstsrch[3] -gt 0 -and $mlstsrch[4] -gt 0 -and $mlstsrch[5] -gt 0) 'Stage MP live-hit status-loop search/proc-stat markers failed.' $gdbStdout
        $mlstp = Get-Ints $stageMPLiveHitStatusProc
        Assert-Condition ($stageMPLiveHitStatusProc.Success -and (($mlstp[0] -band 0x1f) -eq 0x1f) -and $mlstp[2] -ne $mlstp[1] -and $mlstp[2] -ge 37 -and $mlstp[3] -ge 31 -and $mlstp[5] -gt $mlstp[4] -and $mlstp[6] -gt 0) 'Stage MP live-hit status-loop proc/status markers failed.' $gdbStdout
        $mlsth = Get-Ints $stageMPLiveHitStatusHitlag
        Assert-Condition ($stageMPLiveHitStatusHitlag.Success -and $mlsth[0] -eq 6 -and $mlsth[1] -eq 0 -and $mlsth[2] -gt 0 -and $mlsth[3] -eq $mlsth[0] -and $mlsth[4] -eq 1) 'Stage MP live-hit status-loop hitlag countdown markers failed.' $gdbStdout
        $mlstcb = Get-Ints $stageMPLiveHitStatusCallback
        Assert-Condition ($stageMPLiveHitStatusCallback.Success -and $mlstcb[0] -eq 4294967295 -and $mlstcb[1] -eq $mlstp[2] -and $mlstcb[2] -eq 2 -and $mlstcb[3] -eq 1 -and $mlstcb[4] -eq 57 -and $mlstcb[5] -eq 50 -and $mlstcb[6] -eq 23000 -and $mlstcb[7] -gt 0 -and $mlstcb[7] -lt 12000 -and $mlstcb[8] -lt 0 -and $mlstcb[9] -eq 1 -and $mlstcb[10] -eq 1 -and $mlstcb[11] -eq 1 -and $mlstcb[12] -eq 1 -and $mlstcb[13] -eq 1 -and $mlstcb[14] -eq 1 -and $mlstcb[15] -eq 1 -and (($mlstcb[16] -band 0x3ffff) -eq 0x3ffff) -and (($mlstcb[17] -band 0x3f) -eq 0x3f) -and (($mlstcb[18] -band 0x7ff) -eq 0x7ff)) 'Stage MP live-hit status-loop callback tick/physics/interrupt/map-floor-wall-ceil-rwall-cliff-fallmap/expiry markers failed.' $gdbStdout
        $mlstr = Get-Ints $stageMPLiveHitStatusRepeat
        Assert-Condition ($stageMPLiveHitStatusRepeat.Success -and $mlstr[0] -gt 0 -and $mlstr[1] -gt 0 -and (($mlstr[2] -band 0x3f) -eq 0x3f)) 'Stage MP live-hit status-loop repeat suppression markers failed.' $gdbStdout
        $mlstsaf = Get-Ints $stageMPLiveHitStatusSafe
        Assert-Condition ($stageMPLiveHitStatusSafe.Success -and $mlstsaf[0] -ge 0 -and $mlstsaf[1] -ge 0 -and $mlstsaf[2] -eq 1 -and $mlstsaf[3] -eq 1 -and $mlstsaf[4] -eq 0) 'Stage MP live-hit status-loop safety markers failed.' $gdbStdout
        $mpLiveHitSummary = "$mpLiveHitSummary status=$($mlstp[1])->$($mlstp[2])/$($mlstp[3]) hitlag=$($mlsth[0])->$($mlsth[1]) callbacks=$($mlsth[2])/$($mlsth[3])/$($mlsth[4]) update=$($mlstcb[2])->$($mlstcb[3]) phys=$($mlstcb[7])/$($mlstcb[8]) interrupt=$($mlstcb[9]) map=$($mlstcb[10])/$($mlstcb[11]) floor=$($mlstcb[12])/$($mlstcb[13])/$($mlstcb[14])/$($mlstcb[15]) wall=0x$('{0:x}' -f $mlstcb[16]) cliff=0x$('{0:x}' -f $mlstcb[17]) fallmap=0x$('{0:x}' -f $mlstcb[18]) finish=$($mlstcb[4])/$($mlstcb[5]) search=0x$('{0:x}' -f $mlstsrch[0]) repeat=$($mlstr[0])/$($mlstr[1]) gate=0x$('{0:x}' -f $mlstr[2])"
    }
    if ($RequireStageDraw) {
        $sg = Get-Ints $stage
        Assert-Condition ($stage.Success -and $sg[0] -eq 0x46534744 -and $sg[1] -eq 0x46534753 -and (($sg[2] -band 0xfff) -eq 0xfff) -and $sg[3] -eq 0xff -and $sg[4] -eq 2) 'Stage-inclusive gcDrawAll-loop result/mask did not pass.' $gdbStdout
        $cap = Get-Ints $stageCapture
        Assert-Condition ($stageCapture.Success -and $cap[0] -gt 0 -and $cap[1] -gt 0 -and (($cap[2] -band 0xf) -eq 0xf) -and (($cap[3] -band 0xf) -eq 0xf) -and $cap[4] -ge 8 -and $cap[5] -ge $dr[6]) 'Stage-inclusive gcDrawAll capture markers failed.' $gdbStdout
        $dob = Get-Ints $stageDObj
        Assert-Condition ($stageDObj.Success -and $dob[0] -gt 0 -and $dob[1] -ne 0 -and (($dob[2] -bor $dob[3]) -ne 0) -and (($dob[4] -bor $dob[5]) -ne 0)) 'Stage-inclusive DObj/DL-readiness markers failed.' $gdbStdout
        $ss = Get-Ints $stageSafe
        Assert-Condition ($stageSafe.Success -and $ss[0] -eq 1 -and $ss[1] -eq 1 -and $ss[2] -eq 0 -and $ss[3] -eq 0 -and $ss[5] -eq 0) 'Stage-inclusive safety markers failed.' $gdbStdout
        $sp = Get-Ints $stagePixels
        Assert-Condition ($stagePixels.Success -and $sp[0] -gt 0 -and $sp[1] -gt 0) 'Stage-inclusive preview pixel markers failed.' $gdbStdout
        $stageSummary = (" stageCapture=0x{0:x}/0x{1:x} stageDObj=0x{2:x}/0x{3:x} stageDL=0x{4:x}/0x{5:x} stagePixels={6} compat=0x{7:x}" -f $cap[2], $cap[3], $dob[2], $dob[3], $dob[4], $dob[5], $sp[1], $sp[2])
        if ($HardwareTriangles) {
            $hw = Get-Ints $platformHw
            Assert-Condition ($platformHw.Success -and $hw[0] -gt 0 -and $hw[0] -eq $hw[1]) 'Stage-inclusive hardware draw did not flush submitted DS 3D frames.' $gdbStdout
            $shw = Get-Ints $stageHardware
            Assert-Condition ($stageHardware.Success -and $shw[0] -gt 8) 'Stage-inclusive hardware replay did not exceed the old bounded DObj submit slice.' $gdbStdout
            Assert-Condition ($shw[1] -gt 0) 'Stage-inclusive hardware replay did not submit hardware triangles.' $gdbStdout
            $stageSummary = "$stageSummary hwflush=$($hw[0])/$($hw[1]) hwsubmit=$($shw[0]) hwtri=$($shw[1])"
        }
    }
    $stageSummary = "$stageSummary$dashRunSummary$mpDamageRecoverSummary$mpLiveHitSummary"
    if ($RequireStageCollision -and -not $RequireStageFloorEdge) {
        $cl = Get-Ints $stageCollision
        Assert-Condition ($stageCollision.Success -and $cl[0] -eq 0x4653434c -and $cl[1] -eq 0x46534353 -and (($cl[2] -band 0xffff) -eq 0xffff) -and $cl[3] -eq 0xff -and $cl[4] -eq 2) 'Stage collision-loop result/mask did not pass.' $gdbStdout
        $cg = Get-Ints $stageCollisionGeom
        Assert-Condition ($stageCollisionGeom.Success -and $cg[0] -eq 1 -and $cg[1] -eq 1 -and $cg[2] -eq 1 -and $cg[3] -eq 1 -and $cg[4] -gt 0 -and $cg[6] -gt 0 -and $cg[7] -gt 0) 'Stage collision geometry markers failed.' $gdbStdout
        $cp = Get-Ints $stageCollisionProject
        Assert-Condition ($stageCollisionProject.Success -and $cp[0] -gt 0 -and $cp[1] -gt 0 -and $cp[2] -eq 0 -and $cp[3] -eq 0 -and $cp[7] -ge 5) 'Stage collision projection/fallback markers failed.' $gdbStdout
        $cb = Get-Ints $stageCollisionProbes
        Assert-Condition ($stageCollisionProbes.Success -and $cb[0] -ge 3 -and $cb[2] -ge 1 -and $cb[3] -ge 1 -and $cb[4] -gt 0 -and $cb[5] -gt 0 -and $cb[6] -gt 0 -and $cb[7] -gt 0) 'Stage collision probe/player hit markers failed.' $gdbStdout
        $c0 = Get-Ints $stageCollisionP0
        $c1 = Get-Ints $stageCollisionP1
        Assert-Condition ($stageCollisionP0.Success -and $stageCollisionP1.Success -and $c0[0] -ge 0 -and $c1[0] -ge 0 -and $c0[4] -ne 0 -and $c1[4] -ne 0 -and $c0[8] -eq 1 -and $c1[8] -eq 1) 'Stage collision P0/P1 floor markers failed.' $gdbStdout
        $ce = Get-Ints $stageCollisionEdge
        Assert-Condition ($stageCollisionEdge.Success -and $ce[0] -le $ce[2] -and $ce[4] -le $ce[6]) 'Stage collision edge ordering markers failed.' $gdbStdout
        $ck = Get-Ints $stageCollisionKind
        Assert-Condition ($stageCollisionKind.Success -and $ck[1] -gt 0 -and $ck[2] -ge 0 -and $ck[3] -gt $ck[2] -and $ck[4] -eq 0 -and $ck[5] -eq 0 -and $ck[6] -ge $ck[2] -and $ck[6] -lt $ck[3] -and $ck[7] -eq 1 -and $ck[8] -ge $ck[2] -and $ck[8] -lt $ck[3] -and $ck[9] -eq 1 -and $ck[10] -eq 0) 'Stage collision final floor-line classification markers failed.' $gdbStdout
        $cs = Get-Ints $stageCollisionSafe
        Assert-Condition ($stageCollisionSafe.Success -and $cs[0] -eq 1 -and $cs[1] -eq 1 -and $cs[2] -eq 0 -and $cs[3] -eq 0 -and $cs[4] -eq 0 -and $cs[5] -eq 0) 'Stage collision safe counters were not zero.' $gdbStdout
        $stageSummary = "$stageSummary stageCollision=line=$($c0[0])/$($c1[0]) floorRange=$($ck[2])-$($ck[3]) floorLines=$($ck[1]) probes=$($cb[0])/$($cb[1])"
    }
    if ($RequireStageFloorFollow) {
        $ff = Get-Ints $stageFloorFollow
        Assert-Condition ($stageFloorFollow.Success -and $ff[0] -eq 0x46464c50 -and $ff[1] -eq 0x46464c53 -and (($ff[2] -band 0xffff) -eq 0xffff) -and $ff[3] -eq 0xff -and $ff[4] -eq 2) 'Stage floor-follow result/mask did not pass.' $gdbStdout
        $fs = Get-Ints $stageFloorFollowSetup
        Assert-Condition ($stageFloorFollowSetup.Success -and $fs[0] -eq 1 -and $fs[1] -eq 1 -and $fs[2] -eq 1 -and $fs[3] -eq 1 -and $fs[4] -eq 2 -and $fs[5] -eq 2 -and $fs[6] -eq 0 -and $fs[7] -eq 0) 'Stage floor-follow setup/recenter markers failed.' $gdbStdout
        $fu = Get-Ints $stageFloorFollowUpdates
        Assert-Condition ($stageFloorFollowUpdates.Success -and $fu[0] -gt 0 -and $fu[1] -gt 0 -and $fu[2] -gt 0 -and $fu[3] -gt 0 -and $fu[4] -gt 0 -and $fu[5] -eq 0 -and $fu[6] -eq 0 -and $fu[7] -eq 0) 'Stage floor-follow update/project markers failed.' $gdbStdout
        $f0 = Get-Ints $stageFloorFollowP0
        $f1 = Get-Ints $stageFloorFollowP1
        Assert-Condition ($stageFloorFollowP0.Success -and $stageFloorFollowP1.Success -and $f0[0] -ge 0 -and $f1[0] -ge 0 -and $f0[1] -eq 0 -and $f1[1] -eq 0 -and $f0[2] -eq 1 -and $f1[2] -eq 1 -and $f0[5] -ne 0 -and $f1[5] -ne 0 -and $f0[8] -eq 1 -and $f1[8] -eq 1 -and $f0[9] -ne 0 -and $f1[9] -ne 0 -and $f0[10] -eq 10 -and $f1[10] -eq 10 -and $f0[11] -eq 0 -and $f1[11] -eq 0) 'Stage floor-follow P0/P1 final markers failed.' $gdbStdout
        $fd = Get-Ints $stageFloorFollowDrift
        Assert-Condition ($stageFloorFollowDrift.Success -and $fd[0] -le 1000 -and $fd[1] -le 1000 -and $fd[2] -le 1000 -and $fd[3] -le 1000 -and $fd[4] -le 1000) 'Stage floor-follow drift markers failed.' $gdbStdout
        $stageSummary = "$stageSummary floorFollow=line=$($f0[0])/$($f1[0]) updates=$($fu[1])/$($fu[2]) drift=$($fd[0])/$($fd[1]) visits=0x$('{0:x}' -f $f0[9])/0x$('{0:x}' -f $f1[9])"
    }
    if ($RequireStageFloorEdge) {
        $fe = Get-Ints $stageFloorEdge
        Assert-Condition ($stageFloorEdge.Success -and $fe[0] -eq 0x4653454c -and $fe[1] -eq 0x46534553 -and (($fe[2] -band 0xffff) -eq 0xffff) -and $fe[3] -eq 0xff -and $fe[4] -eq 2) 'Stage floor-edge result/mask did not pass.' $gdbStdout
        $fel = Get-Ints $stageFloorEdgeLine
        Assert-Condition ($stageFloorEdgeLine.Success -and $fel[0] -eq 1 -and $fel[1] -eq 1 -and $fel[2] -ge 0 -and $fel[4] -gt $fel[3] -and $fel[5] -gt 0 -and $fel[6] -eq 0 -and $fel[7] -ge 2) 'Stage floor-edge selected floor line markers failed.' $gdbStdout
        $fe0 = Get-Ints $stageFloorEdgeP0
        $fe1 = Get-Ints $stageFloorEdgeP1
        Assert-Condition ($stageFloorEdgeP0.Success -and $stageFloorEdgeP1.Success -and $fe0[0] -gt 0 -and $fe1[0] -gt 0 -and $fe0[2] -gt 0 -and $fe1[2] -gt 0 -and $fe0[4] -eq 1 -and $fe1[4] -eq 1 -and $fe0[5] -eq 1 -and $fe1[5] -eq 1 -and $fe0[6] -eq 1 -and $fe1[6] -eq 1 -and $fe0[7] -ne 0 -and $fe1[7] -ne 0) 'Stage floor-edge P0/P1 approach/floor markers failed.' $gdbStdout
        $fep = Get-Ints $stageFloorEdgeProbes
        Assert-Condition ($stageFloorEdgeProbes.Success -and $fep[0] -ge 2 -and $fep[1] -ge 2 -and $fep[2] -ge 2 -and $fep[3] -ge 2 -and $fep[4] -eq 0) 'Stage floor-edge inside/outside probes failed.' $gdbStdout
        $feq = Get-Ints $stageFloorEdgeQueries
        Assert-Condition ($stageFloorEdgeQueries.Success -and $feq[0] -gt 0 -and $feq[1] -gt 0 -and $feq[2] -gt 0 -and $feq[3] -ge 2 -and $feq[4] -gt 0 -and $feq[5] -gt 0 -and $feq[6] -eq ($feq[4] + $feq[5])) 'Stage floor-edge MP query/deferred-edge markers failed.' $gdbStdout
        $feu = Get-Ints $stageFloorEdgeUpdates
        Assert-Condition ($stageFloorEdgeUpdates.Success -and $feu[0] -gt 0 -and $feu[1] -gt 0 -and $feu[2] -gt 0 -and $feu[3] -gt 0) 'Stage floor-edge update/pre-clamp markers failed.' $gdbStdout
        $fes = Get-Ints $stageFloorEdgeSafe
        Assert-Condition ($stageFloorEdgeSafe.Success -and (@($fes[0..4] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'Stage floor-edge safe counters were not zero.' $gdbStdout
        $stageSummary = "$stageSummary floorEdge=line=$($fel[2]) width=$($fel[5]) dist=$($fe0[1])/$($fe1[1]) delta=$($fe0[2])/$($fe1[2]) probes=$($fep[1])/$($fep[3]) queries=$($feq[0])/$($feq[1])"
    }
    if ($RequireStageMPProcessFloor) {
        $mp = Get-Ints $stageMPProcessFloor
        Assert-Condition ($stageMPProcessFloor.Success -and $mp[0] -eq 0x46534d50 -and $mp[1] -eq 0x46534d53 -and (($mp[2] -band 0xffff) -eq 0xffff) -and $mp[3] -eq 0xff -and $mp[4] -eq 2) 'Stage MP floor-process result/mask did not pass.' $gdbStdout
        $ma = Get-Ints $stageMPProcessFloorAdapter
        Assert-Condition ($stageMPProcessFloorAdapter.Success -and $ma[0] -eq 1 -and $ma[1] -eq 1 -and $ma[2] -gt 0 -and $ma[3] -gt 0) 'Stage MP floor-process adapter markers failed.' $gdbStdout
        $mc = Get-Ints $stageMPProcessFloorCalls
        Assert-Condition ($stageMPProcessFloorCalls.Success -and $mc[0] -gt 0 -and $mc[2] -gt 0 -and $mc[3] -gt 0 -and $mc[4] -gt 0 -and $mc[5] -gt 0 -and $mc[7] -gt 0 -and $mc[8] -gt 0 -and $mc[9] -gt 0) 'Stage MP floor-process source-order call markers failed.' $gdbStdout
        $mf = Get-Ints $stageMPProcessFloorFC
        Assert-Condition ($stageMPProcessFloorFC.Success -and $mf[0] -gt 0 -and $mf[1] -gt 0) 'Stage MP floor-process signed floor-distance markers failed.' $gdbStdout
        $mb = Get-Ints $stageMPProcessFloorProbes
        Assert-Condition ($stageMPProcessFloorProbes.Success -and $mb[0] -ge 1 -and $mb[1] -ge 1 -and $mb[2] -ge 2 -and $mb[3] -ge 2 -and $mb[4] -ge 1 -and $mb[5] -ge 1) 'Stage MP floor-process inside/outside/below probes failed.' $gdbStdout
        $m0 = Get-Ints $stageMPProcessFloorP0
        $m1 = Get-Ints $stageMPProcessFloorP1
        Assert-Condition ($stageMPProcessFloorP0.Success -and $stageMPProcessFloorP1.Success -and $m0[0] -gt 0 -and $m1[0] -gt 0 -and $m0[1] -gt 0 -and $m1[1] -gt 0 -and $m0[2] -eq 0 -and $m1[2] -eq 0 -and $m0[3] -ge 0 -and $m1[3] -ge 0 -and $m0[4] -eq 1 -and $m1[4] -eq 1 -and (($m0[5] -band 0x800) -ne 0) -and (($m1[5] -band 0x800) -ne 0)) 'Stage MP floor-process P0/P1 floor markers failed.' $gdbStdout
        $ms = Get-Ints $stageMPProcessFloorSafe
        Assert-Condition ($stageMPProcessFloorSafe.Success -and $ms[0] -eq 1 -and $ms[1] -eq 0 -and $ms[2] -eq 0 -and $ms[3] -eq 0) 'Stage MP floor-process safe counters failed.' $gdbStdout
        $stageSummary = "$stageSummary mpProcessFloor=test=$($mc[4])/$($mc[5]) project=$($mc[1])/$($mc[2]) probes=$($mb[1])/$($mb[3]) below=$($mb[5]) p0line=$($m0[3]) p1line=$($m1[3]) fc=$($mf[0])/$($mf[1])/$($mf[2])"
    }
    if ($RequireStageMPUpdateFloor) {
        $mu = Get-Ints $stageMPUpdateFloor
        Assert-Condition ($stageMPUpdateFloor.Success -and $mu[0] -eq 0x46554d50 -and $mu[1] -eq 0x46554d53 -and (($mu[2] -band 0xffff) -eq 0xffff) -and $mu[3] -eq 0xff -and $mu[4] -eq 2) 'Stage MP update-main floor-loop result/mask did not pass.' $gdbStdout
        $mua = Get-Ints $stageMPUpdateFloorAdapter
        Assert-Condition ($stageMPUpdateFloorAdapter.Success -and $mua[0] -eq 1 -and $mua[1] -eq 1 -and $mua[2] -gt 0 -and $mua[3] -gt 0) 'Stage MP update-main adapter markers failed.' $gdbStdout
        $muu = Get-Ints $stageMPUpdateFloorUpdate
        Assert-Condition ($stageMPUpdateFloorUpdate.Success -and $muu[0] -gt 0 -and $muu[1] -gt 0 -and $muu[3] -gt 0 -and $muu[4] -gt 0 -and $muu[4] -le 10 -and $muu[5] -gt 0 -and $muu[6] -eq 0 -and $muu[7] -gt 0 -and $muu[8] -gt 0) 'Stage MP update-main source-order update markers failed.' $gdbStdout
        $muc = Get-Ints $stageMPUpdateFloorColl
        Assert-Condition ($stageMPUpdateFloorColl.Success -and $muc[0] -gt 0 -and $muc[1] -gt 0 -and $muc[2] -gt 0 -and $muc[6] -gt 0 -and $muc[7] -gt 0 -and $muc[8] -gt 0 -and (($muc[9] -gt 0) -or $RequireStageMPSweepFloor)) 'Stage MP update-main collision callback markers failed.' $gdbStdout
        $muk = Get-Ints $stageMPUpdateFloorChecks
        Assert-Condition ($stageMPUpdateFloorChecks.Success -and $muk[1] -gt 0 -and $muk[3] -gt 0 -and $muk[3] -gt $muk[5]) 'Stage MP update-main cliff-edge check markers failed.' $gdbStdout
        $mup = Get-Ints $stageMPUpdateFloorProbes
        Assert-Condition ($stageMPUpdateFloorProbes.Success -and $mup[0] -gt 0 -and $mup[1] -gt 0 -and $mup[2] -gt 0 -and $mup[3] -gt 0 -and $mup[4] -gt 0 -and $mup[5] -gt 0 -and $mup[6] -gt 0 -and $mup[7] -gt 1) 'Stage MP update-main inside/outside/below/split probes failed.' $gdbStdout
        $mu0 = Get-Ints $stageMPUpdateFloorP0
        $mu1 = Get-Ints $stageMPUpdateFloorP1
        Assert-Condition ($stageMPUpdateFloorP0.Success -and $stageMPUpdateFloorP1.Success -and $mu0[0] -gt 0 -and $mu1[0] -gt 0 -and $mu0[1] -gt 0 -and $mu1[1] -gt 0 -and $mu0[1] -gt $mu0[2] -and $mu1[1] -gt $mu1[2] -and $mu0[3] -ne 0 -and $mu1[3] -ne 0 -and $mu0[4] -ne $mu0[5] -and $mu1[4] -ne $mu1[5] -and $mu0[7] -ge 0 -and $mu1[7] -ge 0 -and (($mu0[8] -band 0x800) -ne 0) -and (($mu1[8] -band 0x800) -ne 0) -and $mu0[9] -eq 1 -and $mu1[9] -eq 1) 'Stage MP update-main P0/P1 floor markers failed.' $gdbStdout
        $mus = Get-Ints $stageMPUpdateFloorSafe
        Assert-Condition ($stageMPUpdateFloorSafe.Success -and $mus[0] -eq 1 -and (@($mus[2..5] | Where-Object { $_ -ne 0 }).Count -eq 0)) 'Stage MP update-main safe counters failed.' $gdbStdout
        $stageSummary = "$stageSummary mpUpdateFloor=steps=$($muu[3])/$($muu[4]) split=$($muu[5]) probes=$($mup[1])/$($mup[3])/$($mup[5]) p0line=$($mu0[7]) p1line=$($mu1[7]) posdiff=$($mu0[3])/$($mu1[3])"
    }
    if ($RequireStageMPSweepFloor) {
        $sw = Get-Ints $stageMPSweepFloor
        Assert-Condition ($stageMPSweepFloor.Success -and $sw[0] -eq 0x46535750 -and $sw[1] -eq 0x46535753 -and (($sw[2] -band 0xffff) -eq 0xffff) -and $sw[3] -eq 0xff -and $sw[4] -eq 2) 'Stage MP floor-line sweep result/mask did not pass.' $gdbStdout
        $sws = Get-Ints $stageMPSweepFloorSetup
        Assert-Condition ($stageMPSweepFloorSetup.Success -and $sws[0] -eq 1 -and $sws[1] -eq 1 -and $sws[2] -ge 0 -and $sws[3] -ge 0 -and $sws[2] -ne $sws[3] -and $sws[4] -eq 0) 'Stage MP floor-line sweep setup markers failed.' $gdbStdout
        $swc = Get-Ints $stageMPSweepFloorCheck
        Assert-Condition ($stageMPSweepFloorCheck.Success -and $swc[0] -gt 0 -and $swc[1] -gt 0 -and $swc[2] -gt 0 -and $swc[3] -gt 0 -and $swc[4] -gt 0 -and $swc[5] -gt 0 -and $swc[6] -eq 0) 'Stage MP floor-line sweep floor-check markers failed.' $gdbStdout
        $sww = Get-Ints $stageMPSweepFloorSweep
        Assert-Condition ($stageMPSweepFloorSweep.Success -and $sww[0] -gt 0 -and $sww[1] -gt 0 -and $sww[2] -gt 0 -and $sww[3] -gt 0 -and $sww[4] -gt 0 -and $sww[6] -gt 0 -and $sww[7] -gt 0) 'Stage MP floor-line sweep Same/Diff markers failed.' $gdbStdout
        $sw2 = Get-Ints $stageMPSweepFloorSecond
        Assert-Condition ($stageMPSweepFloorSecond.Success -and $sw2[0] -gt 0 -and $sw2[2] -gt 0 -and $sw2[3] -gt 0 -and $sw2[4] -gt 0 -and (($sw2[5] -gt 0) -or $RequireStageMPAdjustFloor) -and $sw2[6] -gt 0 -and $sw2[7] -gt 0) 'Stage MP second-floor branch/landing markers failed.' $gdbStdout
        $swp = Get-Ints $stageMPSweepFloorProbes
        Assert-Condition ($stageMPSweepFloorProbes.Success -and $swp[0] -gt 0 -and $swp[1] -gt 0 -and $swp[2] -gt 0 -and $swp[3] -gt 0 -and $swp[4] -gt 0 -and $swp[5] -gt 0) 'Stage MP floor-line sweep standalone probes failed.' $gdbStdout
        $sw0 = Get-Ints $stageMPSweepFloorP0
        $sw1 = Get-Ints $stageMPSweepFloorP1
        Assert-Condition ($stageMPSweepFloorP0.Success -and $stageMPSweepFloorP1.Success -and $sw0[0] -ge 0 -and $sw1[0] -ge 0 -and $sw0[1] -eq 1 -and $sw1[1] -eq 1 -and $sw0[2] -eq 1 -and $sw1[2] -eq 1) 'Stage MP floor-line sweep final P0/P1 floor markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpSweepFloor=check=$($swc[1])/$($swc[2]) same=$($sww[1])/$($sww[2]) diff=$($sww[4]) line=$($sws[2])->$($sws[3]) second=$($sw2[0])/$($sw2[2]) p0line=$($sw0[0]) p1line=$($sw1[0])"
    }
    if ($RequireStageMPCrossFloor) {
        $cr = Get-Ints $stageMPCrossFloor
        Assert-Condition ($stageMPCrossFloor.Success -and $cr[0] -eq 0x46435250 -and $cr[1] -eq 0x46435253 -and (($cr[2] -band 0xffff) -eq 0xffff) -and $cr[3] -eq 0xff -and $cr[4] -eq 2) 'Stage MP cross-floor result/mask did not pass.' $gdbStdout
        $csf = Get-Ints $stageMPCrossFloorSetup
        Assert-Condition ($stageMPCrossFloorSetup.Success -and $csf[0] -eq 1 -and $csf[1] -eq 1 -and $csf[2] -gt 0 -and $csf[3] -gt 0 -and $csf[4] -eq 0 -and $csf[5] -ge -1 -and $csf[6] -ge 0 -and $csf[5] -ne $csf[6] -and $csf[9] -eq 0) 'Stage MP cross-floor setup/source-target markers failed.' $gdbStdout
        $clv = Get-Ints $stageMPCrossFloorLive
        Assert-Condition ($stageMPCrossFloorLive.Success -and $clv[0] -gt 0 -and $clv[1] -gt 0 -and $clv[2] -gt 0 -and $clv[3] -gt 0 -and $clv[4] -gt 0 -and $clv[5] -gt 0 -and $clv[6] -gt 0) 'Stage MP cross-floor live second-floor markers failed.' $gdbStdout
        $cp0 = Get-Ints $stageMPCrossFloorP0
        $cp1 = Get-Ints $stageMPCrossFloorP1
        Assert-Condition ($stageMPCrossFloorP0.Success -and $stageMPCrossFloorP1.Success -and $cp0[0] -gt 0 -and $cp0[1] -eq $csf[6] -and $cp0[2] -gt 0 -and $cp0[3] -eq 1 -and $cp1[1] -ge 0 -and $cp1[2] -eq 1) 'Stage MP cross-floor P0/P1 final floor markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCrossFloor=line=$($csf[5])->$($csf[6]) live=$($clv[1])/$($clv[2]) accepted=$($clv[3]) p0line=$($cp0[1]) p1line=$($cp1[1])"
    }
    if ($RequireStageMPAdjustFloor) {
        $ad = Get-Ints $stageMPAdjustFloor
        Assert-Condition ($stageMPAdjustFloor.Success -and $ad[0] -eq 0x46454150 -and $ad[1] -eq 0x46454153 -and (($ad[2] -band 0xffff) -eq 0xffff) -and $ad[3] -eq 0xff -and $ad[4] -eq 2) 'Stage MP floor-edge adjust result/mask did not pass.' $gdbStdout
        $ads = Get-Ints $stageMPAdjustFloorSetup
        Assert-Condition ($stageMPAdjustFloorSetup.Success -and $ads[0] -eq 1 -and $ads[1] -eq 1 -and $ads[2] -gt 0 -and $ads[3] -eq 0) 'Stage MP floor-edge adjust setup/run markers failed.' $gdbStdout
        $adc = Get-Ints $stageMPAdjustFloorCheck
        $adw = Get-Ints $stageMPAdjustFloorWall
        $ade = Get-Ints $stageMPAdjustFloorEdge
        if ($RequireStageMPWallFloor) {
            Assert-Condition ($stageMPAdjustFloorCheck.Success -and $adc[0] -gt 0 -and $adc[1] -gt 0 -and $adc[2] -eq 0 -and $adc[3] -eq 0 -and $adc[4] -gt 0 -and $adc[5] -gt 0) 'Stage MP floor-edge adjust blocker check markers failed.' $gdbStdout
            Assert-Condition ($stageMPAdjustFloorWall.Success -and $adw[0] -gt 0 -and $adw[1] -gt 0 -and $adw[2] -eq 0 -and $adw[3] -eq 0 -and $adw[4] -gt 0 -and $adw[5] -gt 0) 'Stage MP floor-edge adjust blocker wall sweep markers failed.' $gdbStdout
        } else {
            Assert-Condition ($stageMPAdjustFloorCheck.Success -and $adc[0] -gt 0 -and $adc[1] -gt 0 -and $adc[2] -eq 0 -and $adc[3] -eq 0 -and $adc[4] -gt 0 -and $adc[5] -gt 0) 'Stage MP floor-edge adjust L/R check markers failed.' $gdbStdout
            Assert-Condition ($stageMPAdjustFloorWall.Success -and $adw[0] -gt 0 -and $adw[1] -gt 0 -and $adw[2] -eq 0 -and $adw[3] -eq 0 -and $adw[4] -gt 0 -and $adw[5] -gt 0) 'Stage MP floor-edge adjust wall sweep markers failed.' $gdbStdout
        }
        if ($RequireStageMPWallFloor) {
            Assert-Condition ($stageMPAdjustFloorEdge.Success -and $ade[0] -gt 0 -and $ade[1] -gt 0 -and $ade[2] -eq 0 -and $ade[3] -eq 0 -and $ade[4] -eq 0 -and $ade[5] -gt 0 -and $sw2[5] -eq 0) 'Stage MP floor-edge adjust blocker edge/no-adjust markers failed.' $gdbStdout
        } elseif ($RequireStageMPEdgeFloor) {
            Assert-Condition ($stageMPAdjustFloorEdge.Success -and $ade[0] -gt 0 -and $ade[1] -gt 0 -and $ade[2] -eq 0 -and $ade[3] -eq 0 -and $ade[4] -eq 0 -and $ade[5] -gt 0 -and $sw2[5] -eq 0) 'Stage MP floor-edge adjust edge/no-adjust markers failed.' $gdbStdout
        } else {
            Assert-Condition ($stageMPAdjustFloorEdge.Success -and $ade[0] -gt 0 -and $ade[1] -gt 0 -and $ade[2] -eq ($ade[0] + $ade[1]) -and $ade[3] -eq 0 -and $ade[4] -eq 0 -and $ade[5] -gt 0 -and $sw2[5] -eq 0) 'Stage MP floor-edge adjust edge/no-adjust markers failed.' $gdbStdout
        }
        $adp = Get-Ints $stageMPAdjustFloorP0P1
        Assert-Condition ($stageMPAdjustFloorP0P1.Success -and $adp[0] -gt 0 -and $adp[1] -eq 0 -and $adp[2] -ge 0 -and $adp[3] -eq -1 -and $adp[4] -eq 1 -and $adp[5] -eq 0 -and $mu1[0] -gt 0 -and $mu1[7] -ge 0 -and $mu1[9] -eq 1) 'Stage MP floor-edge adjust P0 live/P1 maintained final floor markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpAdjustFloor=run=$($ads[2]) checkHit=$($adc[2])/$($adc[3]) checkMiss=$($adc[4])/$($adc[5]) wallHit=$($adw[2])/$($adw[3]) wallMiss=$($adw[4])/$($adw[5]) edge=$($ade[0])/$($ade[1]) adjust=$($ade[3])/$($ade[4]) noAdjust=$($ade[5]) p0line=$($adp[2]) p1maintained=$($mu1[7])"
    }
    if ($RequireStageMPEdgeFloor) {
        $eg = Get-Ints $stageMPEdgeFloor
        Assert-Condition ($stageMPEdgeFloor.Success -and $eg[0] -eq 0x46454750 -and $eg[1] -eq 0x46454753 -and (($eg[2] -band 0xffff) -eq 0xffff) -and $eg[3] -eq 0xff -and $eg[4] -eq 2) 'Stage MP edge-under floor-loop result/mask did not pass.' $gdbStdout
        $egs = Get-Ints $stageMPEdgeFloorSetup
        Assert-Condition ($stageMPEdgeFloorSetup.Success -and $egs[0] -eq 1 -and $egs[1] -eq 1 -and $egs[2] -ge 0 -and $egs[3] -eq 1 -and $egs[4] -eq 0) 'Stage MP edge-under floor-loop setup markers failed.' $gdbStdout
        $ege = Get-Ints $stageMPEdgeFloorEdge
        if ($RequireStageMPWallFloor) {
            Assert-Condition ($stageMPEdgeFloorEdge.Success -and $ege[0] -gt 0 -and $ege[1] -gt 0 -and $ege[2] -gt 0 -and $ege[3] -gt 0) 'Stage MP edge-under query markers failed after wall probe.' $gdbStdout
        } else {
            Assert-Condition ($stageMPEdgeFloorEdge.Success -and $ege[0] -gt 0 -and $ege[1] -gt 0 -and $ege[2] -gt 0 -and $ege[3] -gt 0 -and $ege[4] -eq 0 -and $ege[5] -eq 0) 'Stage MP edge-under query markers failed.' $gdbStdout
        }
        $egl = Get-Ints $stageMPEdgeFloorLine
        Assert-Condition ($stageMPEdgeFloorLine.Success -and $egl[0] -ge 0 -and $egl[1] -ge 0 -and $egl[0] -ne $egl[1] -and (($egl[2] -eq 2) -or ($egl[2] -eq 3)) -and (($egl[3] -eq 2) -or ($egl[3] -eq 3)) -and $egl[4] -eq 0) 'Stage MP edge-under line/kind markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpEdgeFloor=edge=$($egl[0])/$($egl[1]) kind=$($egl[2])/$($egl[3]) calls=$($ege[0])/$($ege[1])"
    }
    if ($RequireStageMPWallFloor) {
        $wl = Get-Ints $stageMPWallFloor
        Assert-Condition ($stageMPWallFloor.Success -and $wl[0] -eq 0x46574c50 -and $wl[1] -eq 0x46574c53 -and (($wl[2] -band 0xffff) -eq 0xffff) -and $wl[3] -eq 0xff -and $wl[4] -eq 2) 'Stage MP wall-blocker floor-loop result/mask did not pass.' $gdbStdout
        $wls = Get-Ints $stageMPWallFloorSetup
        Assert-Condition ($stageMPWallFloorSetup.Success -and $wls[0] -eq 1 -and $wls[1] -eq 1 -and $wls[2] -gt 0 -and $wls[3] -eq 0 -and $wls[4] -gt 0 -and $wls[5] -eq 0) 'Stage MP wall-blocker floor-loop setup/probe markers failed.' $gdbStdout
        $wll = Get-Ints $stageMPWallFloorLine
        Assert-Condition ($stageMPWallFloorLine.Success -and $wll[0] -ge 0 -and $wll[1] -ge 0 -and $wll[3] -ge 0 -and $wll[1] -eq $wll[3] -and (($wll[4] -eq 0 -and $wll[2] -eq 3) -or ($wll[4] -eq 1 -and $wll[2] -eq 2)) -and $wll[5] -eq 0 -and $wll[6] -eq 0 -and $wll[7] -eq 0) 'Stage MP wall-blocker floor-loop line/check/adjust markers failed.' $gdbStdout
        $wlp = Get-Ints $stageMPWallFloorPos
        Assert-Condition ($stageMPWallFloorPos.Success -and $wlp[0] -eq 0 -and $wlp[1] -eq 0 -and $wlp[2] -eq 0 -and $wlp[3] -eq 0 -and $wlp[4] -eq 0 -and $wlp[5] -eq 0) 'Stage MP wall-blocker floor-loop position markers failed.' $gdbStdout
        $wh = Get-Ints $stageMPWallHitScout
        Assert-Condition ($stageMPWallHitScout.Success -and $wh[0] -eq 1 -and $wh[1] -gt 0 -and $wh[2] -gt 0 -and $wh[3] -gt 0 -and (($wh[4] -eq 0 -and $wh[5] -gt 0) -or ($wh[4] -gt 0 -and $wh[6] -eq 0)) -and $wh[6] -eq 0) 'Stage MP wall-hit scout counters failed.' $gdbStdout
        $whl = Get-Ints $stageMPWallHitScoutLine
        $whp = Get-Ints $stageMPWallHitScoutPos
        Assert-Condition ($stageMPWallHitScoutLine.Success -and $stageMPWallHitScoutPos.Success) 'Stage MP wall-hit scout line/pos markers missing.' $gdbStdout
        if ($wh[4] -gt 0) {
            Assert-Condition ($whl[0] -ge 0 -and $whl[1] -ge 0 -and $whl[2] -ge 0 -and $whl[1] -ne $whl[2] -and (($whl[3] -eq 0 -and $whl[4] -eq 3) -or ($whl[3] -eq 1 -and $whl[4] -eq 2)) -and $whl[5] -eq 1 -and (($whp[4] -ne 0) -or ($whp[5] -ne 0))) 'Stage MP wall-hit scout hit markers failed.' $gdbStdout
            $stageSummary = "$stageSummary mpWallFloor=blocked floor=$($wll[0]) edgeWall=$($wll[1]) kind=$($wll[2]) side=$($wll[4]) candidates=$($wls[2]) miss=$($wls[4]) mpWallHitScout=hit floor=$($whl[0]) wall=$($whl[1]) edge=$($whl[2]) side=$($whl[3]) delta=$($whp[4])/$($whp[5])"
        } else {
            Assert-Condition ($whl[0] -eq -1 -and $whl[1] -eq -1 -and $whl[2] -eq -1 -and $whl[5] -eq 0 -and $whp[4] -eq 0 -and $whp[5] -eq 0) 'Stage MP wall-hit scout miss markers failed.' $gdbStdout
            $stageSummary = "$stageSummary mpWallFloor=blocked floor=$($wll[0]) edgeWall=$($wll[1]) kind=$($wll[2]) side=$($wll[4]) candidates=$($wls[2]) miss=$($wls[4]) mpWallHitScout=none floors=$($wh[1]) walls=$($wh[2]) candidates=$($wh[3])"
        }
        $hy = Get-Ints $stageMPWallHyruleScout
        Assert-Condition ($stageMPWallHyruleScout.Success -and $hy[0] -eq 0x4859524c -and $hy[1] -eq 1 -and $hy[2] -eq 1 -and $hy[4] -eq 1 -and $hy[5] -gt 0 -and $hy[6] -gt 0 -and $hy[7] -gt 0 -and (($hy[8] -eq 0 -and $hy[9] -gt 0) -or ($hy[8] -gt 0 -and $hy[10] -eq 0)) -and $hy[10] -eq 0) 'Stage MP Hyrule wall-hit scout counters failed.' $gdbStdout
        $hyl = Get-Ints $stageMPWallHyruleScoutLine
        $hyp = Get-Ints $stageMPWallHyruleScoutPos
        Assert-Condition ($stageMPWallHyruleScoutLine.Success -and $stageMPWallHyruleScoutPos.Success) 'Stage MP Hyrule wall-hit scout line/pos markers missing.' $gdbStdout
        if ($hy[8] -gt 0) {
            Assert-Condition ($hyl[0] -ge 0 -and $hyl[1] -ge 0 -and (($hyl[2] -lt 0) -or ($hyl[1] -ne $hyl[2])) -and (($hyl[3] -eq 0 -and $hyl[4] -eq 3) -or ($hyl[3] -eq 1 -and $hyl[4] -eq 2)) -and $hyl[5] -eq 1 -and (($hyp[4] -ne 0) -or ($hyp[5] -ne 0))) 'Stage MP Hyrule wall-hit scout hit markers failed.' $gdbStdout
            $stageSummary = "$stageSummary hyruleWallHit=hit floor=$($hyl[0]) wall=$($hyl[1]) edge=$($hyl[2]) side=$($hyl[3]) delta=$($hyp[4])/$($hyp[5])"
        } else {
            Assert-Condition ($hyl[0] -eq -1 -and $hyl[1] -eq -1 -and $hyl[2] -eq -1 -and $hyl[5] -eq 0 -and $hyp[4] -eq 0 -and $hyp[5] -eq 0) 'Stage MP Hyrule wall-hit scout miss markers failed.' $gdbStdout
            $stageSummary = "$stageSummary hyruleWallHit=none floors=$($hy[5]) walls=$($hy[6]) candidates=$($hy[7])"
        }
    }
    if ($RequireStageMPWallHitFloor) {
        $whf = Get-Ints $stageMPWallHitFloor
        Assert-Condition ($stageMPWallHitFloor.Success -and $whf[0] -eq 0x46574850 -and $whf[1] -eq 0x46574853 -and (($whf[2] -band 0x3ff) -eq 0x3ff) -and $whf[3] -eq 0xff -and $whf[4] -gt 0) 'Stage MP Hyrule wall-hit floor-loop result/mask did not pass.' $gdbStdout
        $whfs = Get-Ints $stageMPWallHitFloorSetup
        Assert-Condition ($stageMPWallHitFloorSetup.Success -and $whfs[0] -eq 1 -and $whfs[1] -eq 1 -and $whfs[2] -eq 0x4859524c -and $whfs[3] -eq 1 -and $whfs[4] -eq 1 -and $whfs[6] -eq 0) 'Stage MP Hyrule wall-hit floor-loop setup/reloc markers failed.' $gdbStdout
        $whfc = Get-Ints $stageMPWallHitFloorCount
        Assert-Condition ($stageMPWallHitFloorCount.Success -and $whfc[0] -eq 1 -and $whfc[1] -gt 0 -and $whfc[2] -gt 0 -and $whfc[3] -gt 0 -and $whfc[4] -gt 0) 'Stage MP Hyrule wall-hit floor-loop counter markers failed.' $gdbStdout
        $whfl = Get-Ints $stageMPWallHitFloorLine
        Assert-Condition ($stageMPWallHitFloorLine.Success -and $whfl[0] -eq 5 -and $whfl[1] -eq 13 -and $whfl[2] -eq 12 -and $whfl[3] -eq 0 -and $whfl[4] -eq 3 -and $whfl[5] -eq 1) 'Stage MP Hyrule wall-hit floor-loop line markers failed.' $gdbStdout
        $whfp = Get-Ints $stageMPWallHitFloorPos
        Assert-Condition ($stageMPWallHitFloorPos.Success -and $whfp[4] -eq -1600 -and $whfp[5] -eq -388) 'Stage MP Hyrule wall-hit floor-loop position markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpWallHitFloor=pass floor=$($whfl[0]) wall=$($whfl[1]) edge=$($whfl[2]) side=$($whfl[3]) mapNodes=$($whfs[5]) delta=$($whfp[4])/$($whfp[5])"
    }
    if ($RequireStageMPWallCopyFloor) {
        $wcf = Get-Ints $stageMPWallCopyFloor
        Assert-Condition ($stageMPWallCopyFloor.Success -and $wcf[0] -eq 0x46574350 -and $wcf[1] -eq 0x46574353 -and (($wcf[2] -band 0x3ff) -eq 0x3ff) -and $wcf[3] -eq 0xff -and $wcf[4] -eq 1) 'Stage MP wall-copy floor-loop result/mask did not pass.' $gdbStdout
        $wcb = Get-Ints $stageMPWallCopyFloorBase
        Assert-Condition ($stageMPWallCopyFloorBase.Success -and $wcb[0] -eq 1 -and $wcb[1] -eq 1 -and $wcb[2] -eq 1 -and $wcb[3] -eq 1 -and $wcb[4] -eq 1 -and $wcb[5] -eq 1) 'Stage MP wall-copy process/copyback counts failed.' $gdbStdout
        $wcs = Get-Ints $stageMPWallCopyFloorSrc
        Assert-Condition ($stageMPWallCopyFloorSrc.Success -and $wcs[0] -eq 5 -and $wcs[1] -eq 13 -and $wcs[2] -eq 12 -and $wcs[3] -eq 0) 'Stage MP wall-copy source wall-hit markers failed.' $gdbStdout
        $wcp = Get-Ints $stageMPWallCopyFloorPos
        Assert-Condition ($stageMPWallCopyFloorPos.Success -and $wcp[4] -eq -1600 -and $wcp[5] -eq -388) 'Stage MP wall-copy position delta markers failed.' $gdbStdout
        $wcstate = Get-Ints $stageMPWallCopyFloorState
        Assert-Condition ($stageMPWallCopyFloorState.Success -and $wcstate[0] -eq 1 -and (($wcstate[1] -band 0x800) -ne 0) -and $wcstate[2] -eq 0 -and $wcstate[3] -eq 0 -and $wcstate[4] -eq $wcstate[6] -and $wcstate[5] -eq $wcstate[7]) 'Stage MP wall-copy final state/P1 guard markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpWallCopy=pass floor=$($wcs[0]) wall=$($wcs[1]) edge=$($wcs[2]) delta=$($wcp[4])/$($wcp[5])"
    }
    if ($RequireStageMPPassFloor) {
        $mpp = Get-Ints $stageMPPassFloor
        Assert-Condition ($stageMPPassFloor.Success -and $mpp[0] -eq 0x46505050 -and $mpp[1] -eq 0x46505053 -and (($mpp[2] -band 0x7ff) -eq 0x7ff) -and $mpp[3] -eq 0xff -and $mpp[4] -eq 1) 'Stage MP pass-through floor-loop result/mask did not pass.' $gdbStdout
        $mppb = Get-Ints $stageMPPassFloorBase
        Assert-Condition ($stageMPPassFloorBase.Success -and $mppb[0] -eq 1 -and $mppb[1] -eq 1 -and $mppb[2] -gt 0 -and $mppb[3] -eq 1 -and $mppb[4] -eq 0 -and $mppb[5] -eq 0) 'Stage MP pass-through base/candidate markers failed.' $gdbStdout
        $mppl = Get-Ints $stageMPPassFloorLine
        Assert-Condition ($stageMPPassFloorLine.Success -and $mppl[0] -ge 0 -and (($mppl[1] -band 0x4000) -ne 0) -and $mppl[2] -eq 1) 'Stage MP pass-through line flag markers failed.' $gdbStdout
        $mppr = Get-Ints $stageMPPassFloorRoute
        Assert-Condition ($stageMPPassFloorRoute.Success -and $mppr[0] -eq 2 -and $mppr[1] -eq 1 -and $mppr[2] -eq 1 -and $mppr[3] -eq 1 -and $mppr[4] -eq 1 -and $mppr[5] -eq 0) 'Stage MP pass-through route/callback markers failed.' $gdbStdout
        $mppp = Get-Ints $stageMPPassFloorProcess
        Assert-Condition ($stageMPPassFloorProcess.Success -and $mppp[0] -eq 1 -and $mppp[1] -eq 1 -and $mppp[2] -eq 1) 'Stage MP pass-through process markers failed.' $gdbStdout
        $mppp1 = Get-Ints $stageMPPassFloorP1
        Assert-Condition ($stageMPPassFloorP1.Success -and $mppp1[0] -eq $mppp1[2] -and $mppp1[1] -eq $mppp1[3] -and $mppp1[4] -eq 1) 'Stage MP pass-through P1 guard markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpPass=line=$($mppl[0]) flags=0x$('{0:x}' -f $mppl[1]) route=$($mppr[0]) same=$($mppr[1]) diff=$($mppr[2]) cb=$($mppr[3])/$($mppr[4])/$($mppr[5])"
    }
    if ($RequireStageMPPlatformFloor) {
        $plf = Get-Ints $stageMPPlatformFloor
        Assert-Condition ($stageMPPlatformFloor.Success -and $plf[0] -eq 0x46504c50 -and $plf[1] -eq 0x46504c53 -and (($plf[2] -band 0xff) -eq 0xff) -and $plf[3] -eq 0xff) 'Stage MP platform floor-loop result/mask did not pass.' $gdbStdout
        $plb = Get-Ints $stageMPPlatformFloorBase
        Assert-Condition ($stageMPPlatformFloorBase.Success -and $plb[0] -eq 1 -and $plb[1] -eq 1 -and $plb[2] -eq 1 -and $plb[3] -eq 0) 'Stage MP platform base/probe markers failed.' $gdbStdout
        $pll = Get-Ints $stageMPPlatformFloorLine
        Assert-Condition ($stageMPPlatformFloorLine.Success -and $pll[0] -ge 0 -and (($pll[1] -band 0x4000) -ne 0) -and $pll[2] -eq 1 -and $pll[4] -gt 0) 'Stage MP platform line/yakumono markers failed.' $gdbStdout
        $pld = Get-Ints $stageMPPlatformFloorDObj
        Assert-Condition ($stageMPPlatformFloorDObj.Success -and (($pld[3] -eq 1) -or ($pld[4] -ne 0))) 'Stage MP platform predicate/blocker classification failed.' $gdbStdout
        if ($RequireStageMPPlatformActiveFloor) {
            Assert-Condition ($pld[0] -eq 1 -and $pld[1] -eq 1 -and $pld[3] -eq 1 -and $pld[4] -eq 0) 'Stage MP platform active yakumono proof did not activate the original predicate.' $gdbStdout
        }
        $platformState = if ($pld[3] -eq 1) { 'active' } else { "deferred=0x$('{0:x}' -f $pld[4])" }
        $stageSummary = "$stageSummary mpPlatform=line=$($pll[0]) yak=$($pll[3]) dobj=$($pld[0]) status=$($pld[1]) anim=$($pld[2]) $platformState"
    }
    if ($RequireStageMPPlatformTickFloor) {
        $pt = Get-Ints $stageMPPlatformTickFloor
        Assert-Condition ($stageMPPlatformTickFloor.Success -and $pt[0] -eq 0x46505450 -and $pt[1] -eq 0x46505453 -and (($pt[2] -band 0xff) -eq 0xff) -and $pt[3] -eq 0xff -and $pt[4] -eq 1) 'Stage MP platform tick result/mask did not pass.' $gdbStdout
        $pts = Get-Ints $stageMPPlatformTickFloorStep
        Assert-Condition ($stageMPPlatformTickFloorStep.Success -and $pts[0] -eq 1 -and $pts[1] -eq 1 -and $pts[2] -gt 0 -and $pts[3] -eq 1 -and $pts[4] -eq 0 -and $pts[6] -eq ($pts[5] + 1) -and $pts[7] -eq 1 -and $pts[8] -eq 1 -and $pts[9] -eq 1 -and $pts[10] -eq 1) 'Stage MP platform tick step markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpPlatformTick=tic=$($pts[5])->$($pts[6]) setOn=$($pts[2]) advance=$($pts[3]) status=$($pts[8])/$($pts[9])"
    }
    if ($RequireStageMPPassInputLoop) {
        $pi = Get-Ints $stageMPPassInputLoop
        Assert-Condition ($stageMPPassInputLoop.Success -and $pi[0] -eq 0x46504950 -and $pi[1] -eq 0x46504953 -and (($pi[2] -band 0x7ff) -eq 0x7ff) -and $pi[3] -eq 0x7ff -and $pi[4] -eq 1) 'Stage MP pass-input result/mask did not pass.' $gdbStdout
        $pis = Get-Ints $stageMPPassInputLoopSetup
        Assert-Condition ($stageMPPassInputLoopSetup.Success -and $pis[0] -eq 1 -and $pis[1] -eq 1 -and $pis[2] -eq 1 -and $pis[3] -eq 1 -and $pis[4] -eq 1 -and $pis[5] -eq 3 -and $pis[6] -eq 1 -and $pis[7] -eq 1 -and $pis[8] -eq 1 -and $pis[9] -eq 1 -and $pis[10] -eq 0) 'Stage MP pass-input setup markers failed.' $gdbStdout
        $pist = Get-Ints $stageMPPassInputLoopState
        Assert-Condition ($stageMPPassInputLoopState.Success -and $pist[0] -ge 0 -and (($pist[1] -band 0x4000) -ne 0) -and $pist[2] -le -53 -and $pist[3] -lt 4 -and $pist[4] -eq 254 -and $pist[5] -eq 10 -and $pist[6] -eq 28 -and $pist[7] -eq 33 -and $pist[8] -eq 1 -and $pist[9] -eq $pist[0] -and $pist[10] -eq 3 -and $pist[11] -eq 0) 'Stage MP pass-input state/status markers failed.' $gdbStdout
        $pirv = Get-Ints $stageMPPassInputLoopSquatRv
        Assert-Condition ($stageMPPassInputLoopSquatRv.Success -and $pirv[0] -eq 1 -and $pirv[1] -eq 1 -and $pirv[2] -eq 1 -and $pirv[3] -eq 1 -and $pirv[4] -eq 29 -and $pirv[5] -eq 30 -and $pirv[6] -eq 10 -and (($pirv[7] -band 0xf) -eq 0xf) -and $pirv[8] -eq 0) 'Stage MP pass-input SquatWait/SquatRv markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpPassInput=line=$($pist[0]) flags=0x$('{0:x}' -f $pist[1]) squat=$($pist[6]) pass=$($pist[7]) rv=$($pirv[4])->$($pirv[5])->$($pirv[6]) ignore=$($pist[9])"
    }
    if ($RequireStageMPPlatformPosFloor) {
        $pp = Get-Ints $stageMPPlatformPosFloor
        Assert-Condition ($stageMPPlatformPosFloor.Success -and $pp[0] -eq 0x4650504f -and $pp[1] -eq 0x46505053 -and (($pp[2] -band 0xff) -eq 0xff) -and $pp[3] -eq 0xff -and $pp[4] -eq 1) 'Stage MP platform position result/mask did not pass.' $gdbStdout
        $pps = Get-Ints $stageMPPlatformPosFloorSetup
        Assert-Condition ($stageMPPlatformPosFloorSetup.Success -and $pps[0] -eq 1 -and $pps[1] -eq 1 -and $pps[2] -eq 1 -and $pps[3] -eq 0 -and $pps[4] -ge 0 -and $pps[6] -eq 1 -and $pps[7] -eq 1 -and $pps[8] -eq 1 -and $pps[9] -eq 1) 'Stage MP platform position setup markers failed.' $gdbStdout
        $ppv = Get-Ints $stageMPPlatformPosFloorVec
        Assert-Condition ($stageMPPlatformPosFloorVec.Success -and $ppv[6] -eq $ppv[3] -and $ppv[7] -eq $ppv[4] -and $ppv[8] -eq $ppv[5] -and $ppv[9] -eq ($ppv[3] - $ppv[0]) -and $ppv[10] -eq ($ppv[4] - $ppv[1]) -and $ppv[11] -eq ($ppv[5] - $ppv[2]) -and $ppv[9] -eq 12000 -and $ppv[10] -eq -4000 -and $ppv[11] -eq 2000) 'Stage MP platform position vector/speed markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpPlatformPos=line=$($pps[4]) yak=$($pps[5]) delta=$($ppv[9])/$($ppv[10])/$($ppv[11])"
    }
    if ($RequireStageMPPlatformSpeedFloor) {
        $ps = Get-Ints $stageMPPlatformSpeedFloor
        Assert-Condition ($stageMPPlatformSpeedFloor.Success -and $ps[0] -eq 0x46505350 -and $ps[1] -eq 0x46505353 -and (($ps[2] -band 0x3fff) -eq 0x3fff) -and $ps[3] -eq 0xff -and $ps[4] -eq 1) 'Stage MP platform speed result/mask did not pass.' $gdbStdout
        $pss = Get-Ints $stageMPPlatformSpeedFloorSetup
        Assert-Condition ($stageMPPlatformSpeedFloorSetup.Success -and $pss[0] -eq 1 -and $pss[1] -eq 1 -and $pss[2] -eq 1 -and $pss[3] -eq 0 -and $pss[4] -ge 0 -and $pss[6] -eq 1 -and $pss[7] -eq 1) 'Stage MP platform speed setup markers failed.' $gdbStdout
        $psv = Get-Ints $stageMPPlatformSpeedFloorVec
        Assert-Condition ($stageMPPlatformSpeedFloorVec.Success -and $psv[0] -eq 12000 -and $psv[1] -eq -4000 -and $psv[2] -eq 2000 -and $psv[3] -eq $psv[0] -and $psv[4] -eq $psv[1] -and $psv[5] -eq $psv[2]) 'Stage MP platform speed vector markers failed.' $gdbStdout
        $psd = Get-Ints $stageMPPlatformSpeedFloorDynamic
        Assert-Condition ($stageMPPlatformSpeedFloorDynamic.Success -and $psd[0] -gt 0 -and $psd[1] -eq 1 -and $psd[2] -gt 0 -and $psd[3] -eq $pss[4] -and $psd[4] -eq $pss[5] -and $psd[5] -eq $psv[3] -and $psd[6] -eq $psv[4]) 'Stage MP platform speed dynamic floor sweep markers failed.' $gdbStdout
        $psc = Get-Ints $stageMPPlatformSpeedFloorDynamicCeil
        Assert-Condition ($stageMPPlatformSpeedFloorDynamicCeil.Success -and $psc[0] -eq 1 -and $psc[1] -gt 0 -and $psc[2] -ge 0 -and $psc[3] -eq $pss[5]) 'Stage MP platform speed dynamic ceil sweep markers failed.' $gdbStdout
        $psw = Get-Ints $stageMPPlatformSpeedFloorDynamicWall
        Assert-Condition ($stageMPPlatformSpeedFloorDynamicWall.Success -and $psw[0] -eq 1 -and $psw[1] -gt 0 -and $psw[2] -ge 0 -and $psw[3] -eq $pss[5] -and ($psw[4] -eq 2 -or $psw[4] -eq 3)) 'Stage MP platform speed dynamic wall sweep markers failed.' $gdbStdout
        $pspw = Get-Ints $stageMPPlatformSpeedFloorProcessWall
        Assert-Condition ($stageMPPlatformSpeedFloorProcessWall.Success -and $pspw[0] -eq 1 -and $pspw[1] -gt 0 -and $pspw[2] -ge 0 -and ($pspw[3] -eq 2 -or $pspw[3] -eq 3) -and $pspw[4] -ne 0 -and $pspw[5] -gt 0) 'Stage MP platform speed process-wall markers failed.' $gdbStdout
        $psa = Get-Ints $stageMPPlatformSpeedFloorAnim
        Assert-Condition ($stageMPPlatformSpeedFloorAnim.Success -and $psa[0] -eq 1 -and $psa[2] -eq (($psa[1] + 1) -band 0xffff) -and $psa[3] -eq 2 -and $psa[4] -eq 2 -and $psa[5] -eq $psv[3] -and $psa[6] -eq $psv[4] -and $psa[7] -eq $psv[5]) 'Stage MP platform animation speed markers failed.' $gdbStdout
        $psb = Get-Ints $stageMPPlatformSpeedFloorBounds
        Assert-Condition ($stageMPPlatformSpeedFloorBounds.Success -and $psb[0] -eq 1 -and (($psb[1] -ne 0) -or ($psb[2] -ne 0) -or ($psb[3] -ne 0) -or ($psb[4] -ne 0))) 'Stage MP platform bounds recompute markers failed.' $gdbStdout
        $pssa = Get-Ints $stageMPPlatformSpeedFloorStageAnim
        Assert-Condition ($stageMPPlatformSpeedFloorStageAnim.Success -and (($pssa[0] -band 0x91) -eq 0x91) -and $pssa[3] -eq 1) 'Stage MP platform layer animation blocker markers failed.' $gdbStdout
        $stageAnimReady = 0
        if ((($pssa[0] -band 0x6) -ne 0) -and (($pssa[1] + $pssa[2]) -gt 0)) {
            $stageAnimReady = 1
        }
        $inia = Get-Ints $stageInishieAsset
        Assert-Condition ($stageInishieAsset.Success -and $inia[0] -eq 0x494e4952 -and $inia[1] -eq 0x14 -and $inia[2] -eq 1 -and $inia[3] -eq 1) 'Stage Inishie asset markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpPlatformSpeed=line=$($pss[4]) yak=$($pss[5]) read=$($psv[3])/$($psv[4]) dyn=$($psd[0])/$($psd[2]) ceil=$($psc[0])/$($psc[1]) wall=$($psw[0])/$($psw[1]) procwall=$($pspw[0])/$($pspw[1]) anim=$($psa[0]) bounds=$($psb[0]) stageanim=$stageAnimReady/0x$('{0:x}' -f $pssa[0]) inishieAsset=header/geometry nodes=$($inia[4])"
    }
    if ($RequireStageInishieScaleLoop) {
        $is = Get-Ints $stageInishieScale
        Assert-Condition ($stageInishieScale.Success -and $is[0] -eq 0x46495350 -and $is[1] -eq 0x46495353 -and (($is[2] -band 0x7ff) -eq 0x7ff) -and $is[3] -eq 0xff -and $is[4] -eq 4) 'Stage Inishie scale result/mask did not pass.' $gdbStdout
        $iss = Get-Ints $stageInishieScaleSetup
        Assert-Condition ($stageInishieScaleSetup.Success -and $iss[0] -eq 1 -and $iss[1] -eq 1 -and $iss[2] -eq 1 -and $iss[3] -eq 2 -and $iss[4] -eq 4 -and $iss[5] -eq 2 -and $iss[6] -eq 0 -and $iss[7] -eq 0xf -and $iss[8] -eq 0x3 -and $iss[9] -eq 0x3 -and $iss[10] -eq 0x3) 'Stage Inishie scale setup/call markers failed.' $gdbStdout
        $issource = Get-Ints $stageInishieScaleSource
        if ($stageInishieScaleSource.Success -and $issource[0] -eq 13) {
            Assert-Condition ((($issource[3] -band 0x1ff00) -eq 0x1ff00)) 'Stage Inishie source setup did not load/validate/fix up raw StageInishieFile3 data.' $gdbStdout
            $isd = Get-Ints $stageInishieScaleDisplay
            Assert-Condition ($stageInishieScaleDisplay.Success -and (($isd[0] -band 0xff) -eq 0xff) -and $isd[1] -eq 4 -and $isd[2] -gt 0 -and $isd[3] -gt 0 -and $isd[4] -eq 0) 'Stage Inishie source display DObj/DL scan markers failed.' $gdbStdout
            $ism = Get-Ints $stageInishieScaleMaterial
            Assert-Condition ($stageInishieScaleMaterial.Success -and (($ism[0] -band 0x3f) -eq 0x3f) -and $ism[1] -gt 0 -and $ism[2] -ne 0 -and $ism[3] -ne 0) 'Stage Inishie source material/texture markers failed.' $gdbStdout
            $isp = Get-Ints $stageInishieScalePreview
            Assert-Condition ($stageInishieScalePreview.Success -and (($isp[0] -band 0x3d) -eq 0x3d) -and $isp[1] -eq 4 -and $isp[2] -gt 0 -and $isp[3] -gt 0 -and $isp[4] -gt 0 -and $isp[5] -gt 0 -and $isp[6] -eq 1 -and $isp[7] -eq 0) 'Stage Inishie source preview markers failed.' $gdbStdout
        }
        $isl = Get-Ints $stageInishieScaleLines
        Assert-Condition ($stageInishieScaleLines.Success -and $isl[0] -eq 1 -and $isl[1] -eq 2 -and $isl[2] -eq 5 -and $isl[3] -eq 6 -and $isl[4] -eq 0 -and $isl[5] -eq 0) 'Stage Inishie scale line/kind/status markers failed.' $gdbStdout
        $isa = Get-Ints $stageInishieScaleAlt
        Assert-Condition ($stageInishieScaleAlt.Success -and $isa[0] -eq 80000 -and $isa[1] -eq 64000 -and $isa[4] -eq $isa[2] -and $isa[5] -eq $isa[3] -and $isa[6] -eq ($isa[2] + 64000) -and $isa[7] -eq ($isa[3] - 64000) -and $isa[8] -eq -8000 -and $isa[9] -eq 8000) 'Stage Inishie scale altitude/platform-speed markers failed.' $gdbStdout
        $isf = Get-Ints $stageInishieScaleFall
        Assert-Condition ($stageInishieScaleFall.Success -and $isf[0] -eq 4 -and $isf[1] -eq 2 -and $isf[2] -eq 1 -and $isf[3] -eq 2 -and $isf[4] -eq 1100000 -and $isf[5] -eq 0 -and $isf[6] -eq ($isa[2] + 1097000) -and $isf[7] -eq ($isa[3] - 1103000) -and $isf[8] -eq -3000 -and $isf[9] -eq -3000) 'Stage Inishie scale forced Fall/Sleep markers failed.' $gdbStdout
        $isst = Get-Ints $stageInishieScaleStep
        Assert-Condition ($stageInishieScaleStep.Success -and $isst[0] -eq 4 -and $isst[1] -eq 0x3 -and $isst[2] -eq 2 -and $isst[3] -eq 0x3 -and $isst[4] -eq 1 -and $isst[5] -eq 0 -and $isst[6] -eq 3 -and $isst[7] -eq 0 -and $isst[8] -eq 0 -and $isst[9] -eq $isa[2] -and $isst[10] -eq $isa[3] -and $isst[11] -eq -1097000 -and $isst[12] -eq 1103000) 'Stage Inishie scale Sleep/Retract markers failed.' $gdbStdout
        $sourceSummary = ''
        if ($stageInishieScaleSource.Success -and $issource[0] -eq 13 -and $stageInishieScaleDisplay.Success) {
            $isd = Get-Ints $stageInishieScaleDisplay
            $ism = Get-Ints $stageInishieScaleMaterial
            $isp = Get-Ints $stageInishieScalePreview
            $sourceSummary = " sourceDL=0x$('{0:x}' -f $isd[0]) cmds=$($isd[2]) tris=$($isd[3]) tex=0x$('{0:x}' -f $ism[0]) preview=0x$('{0:x}' -f $isp[0]) px=$($isp[5])"
        }
        $stageSummary = "$stageSummary inishieScale=ticks=$($iss[3]) lines=$($isl[0])/$($isl[1]) alt=$($isa[0])->$($isa[1]) y=$($isa[4])/$($isa[5])->$($isa[6])/$($isa[7]) speed=$($isa[8])/$($isa[9]) fall=$($isf[2])->$($isf[3])/$($isf[5]) step=$($isst[6])->$($isst[7])/$($isst[8])$sourceSummary"
    }
    if ($RequireStageMPStaleFloor) {
        $sl = Get-Ints $stageMPStaleFloor
        Assert-Condition ($stageMPStaleFloor.Success -and $sl[0] -eq 0x46535450 -and $sl[1] -eq 0x46535453 -and (($sl[2] -band 0xffff) -eq 0xffff) -and $sl[3] -eq 0xff -and $sl[4] -eq 2) 'Stage MP stale-floor result/mask did not pass.' $gdbStdout
        $sls = Get-Ints $stageMPStaleFloorSetup
        Assert-Condition ($stageMPStaleFloorSetup.Success -and $sls[0] -eq 1 -and $sls[1] -eq 1 -and $sls[2] -gt 0 -and $sls[3] -gt 0 -and $sls[4] -eq 0 -and $sls[5] -ge 0 -and $sls[6] -ge 0 -and $sls[5] -ne $sls[6] -and $sls[9] -eq 0) 'Stage MP stale-floor setup/source-target markers failed.' $gdbStdout
        $sll = Get-Ints $stageMPStaleFloorLive
        Assert-Condition ($stageMPStaleFloorLive.Success -and $sll[0] -gt 0 -and $sll[1] -gt 0 -and $sw2[2] -gt 0 -and $sll[3] -gt 0 -and $sll[4] -gt 0 -and $sll[5] -gt 0 -and $sll[6] -gt 0) 'Stage MP stale-floor live second-floor markers failed.' $gdbStdout
        $slp = Get-Ints $stageMPStaleFloorP0P1
        Assert-Condition ($stageMPStaleFloorP0P1.Success -and $slp[0] -eq $sls[6] -and $slp[1] -ge 0 -and $slp[2] -gt 0 -and $slp[3] -eq 1 -and $slp[4] -eq 1) 'Stage MP stale-floor P0/P1 final floor markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpStaleFloor=line=$($sls[5])->$($sls[6]) live=$($sll[1])/$($sll[2]) accepted=$($sll[3]) p0line=$($slp[0]) p1line=$($slp[1])"
    }
    if ($RequireStageMPLiveStaleFloor) {
        $ll = Get-Ints $stageMPLiveStaleFloor
        Assert-Condition ($stageMPLiveStaleFloor.Success -and $ll[0] -eq 0x464c5350 -and $ll[1] -eq 0x464c5353 -and (($ll[2] -band 0xffff) -eq 0xffff) -and $ll[3] -eq 0xff -and $ll[4] -eq 2) 'Stage MP live-stale-floor result/mask did not pass.' $gdbStdout
        $lls = Get-Ints $stageMPLiveStaleFloorSetup
        Assert-Condition ($stageMPLiveStaleFloorSetup.Success -and $lls[0] -eq 1 -and $lls[1] -eq 1 -and $lls[2] -gt 0 -and $lls[3] -gt 0 -and $lls[4] -eq 0 -and $lls[5] -ge 0 -and $lls[6] -ge 0 -and $lls[5] -ne $lls[6] -and $lls[9] -gt 0 -and $lls[10] -eq 0) 'Stage MP live-stale-floor setup/source-target markers failed.' $gdbStdout
        $lll = Get-Ints $stageMPLiveStaleFloorLive
        Assert-Condition ($stageMPLiveStaleFloorLive.Success -and $lll[0] -gt 0 -and $lll[1] -gt 0 -and $lll[3] -gt 0 -and $lll[4] -gt 0 -and $lll[5] -gt 0 -and $lll[6] -gt 0) 'Stage MP live-stale-floor live second-floor markers failed.' $gdbStdout
        $llp = Get-Ints $stageMPLiveStaleFloorP0P1
        Assert-Condition ($stageMPLiveStaleFloorP0P1.Success -and $llp[0] -eq $lls[6] -and $llp[1] -ge 0 -and $llp[2] -gt 0 -and $llp[3] -eq 1 -and $llp[4] -eq 1) 'Stage MP live-stale-floor P0/P1 final floor markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpLiveStaleFloor=line=$($lls[5])->$($lls[6]) selected=$($lls[9]) live=$($lll[1])/$($lll[2]) accepted=$($lll[3]) p0line=$($llp[0]) p1line=$($llp[1])"
    }
    if ($RequireStageMPMotionStaleFloor) {
        $ml = Get-Ints $stageMPMotionStaleFloor
        Assert-Condition ($stageMPMotionStaleFloor.Success -and $ml[0] -eq 0x464d5350 -and $ml[1] -eq 0x464d5353 -and (($ml[2] -band 0xffff) -eq 0xffff) -and $ml[3] -eq 0xff -and $ml[4] -eq 2) 'Stage MP motion-stale-floor result/mask did not pass.' $gdbStdout
        $mls = Get-Ints $stageMPMotionStaleFloorSetup
        Assert-Condition ($stageMPMotionStaleFloorSetup.Success -and $mls[0] -eq 1 -and $mls[1] -eq 1 -and $mls[2] -gt 0 -and $mls[3] -gt 0 -and $mls[4] -eq 0 -and $mls[5] -ge 0 -and $mls[6] -ge 0 -and $mls[5] -ne $mls[6] -and $mls[9] -eq 1 -and $mls[10] -gt 0 -and $mls[11] -gt 0 -and $mls[12] -eq 0) 'Stage MP motion-stale-floor setup/update markers failed.' $gdbStdout
        $mlpos = Get-Ints $stageMPMotionStaleFloorPos
        Assert-Condition ($stageMPMotionStaleFloorPos.Success -and (($mlpos[0] -ne $mlpos[2]) -or ($mlpos[1] -ne $mlpos[3])) -and ([Math]::Abs($mlpos[4] - $mlpos[2]) -le 1) -and ([Math]::Abs($mlpos[5] - $mlpos[3]) -le 1)) 'Stage MP motion-stale-floor position copyback markers failed.' $gdbStdout
        $mlp = Get-Ints $stageMPMotionStaleFloorP0P1
        Assert-Condition ($stageMPMotionStaleFloorP0P1.Success -and $mlp[0] -eq $mls[6] -and $mlp[1] -ge 0 -and $mlp[2] -eq 1 -and $mlp[3] -eq 1) 'Stage MP motion-stale-floor P0/P1 final floor markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpMotionStaleFloor=line=$($mls[5])->$($mls[6]) mutation=$($mls[9]) update=$($mls[10]) match=$($mls[11]) p0line=$($mlp[0]) p1line=$($mlp[1]) pos=$($mlpos[0]),$($mlpos[1])->$($mlpos[4]),$($mlpos[5])"
    }
    if ($RequireStageMPCliffStatusFloor) {
        $cf = Get-Ints $stageMPCliffStatusFloor
        Assert-Condition ($stageMPCliffStatusFloor.Success -and $cf[0] -eq 0x46435350 -and $cf[1] -eq 0x46435353 -and (($cf[2] -band 0x3ff) -eq 0x3ff) -and $cf[3] -eq 0xff -and $cf[4] -eq 2) 'Stage MP cliff-status floor-loop result/mask did not pass.' $gdbStdout
        $cfs = Get-Ints $stageMPCliffStatusFloorSetup
        Assert-Condition ($stageMPCliffStatusFloorSetup.Success -and $cfs[0] -eq 1 -and $cfs[1] -eq 1 -and $cfs[2] -eq 2 -and $cfs[3] -eq 2 -and $cfs[4] -eq 0 -and $cfs[5] -eq 1 -and $cfs[6] -eq 1 -and $cfs[7] -eq 0) 'Stage MP cliff-status branch markers failed.' $gdbStdout
        $cft = Get-Ints $stageMPCliffStatusFloorStatus
        Assert-Condition ($stageMPCliffStatusFloorStatus.Success -and $cft[0] -eq 1 -and $cft[1] -eq 1 -and $cft[2] -eq 2 -and $cft[3] -eq 1) 'Stage MP cliff-status status-call markers failed.' $gdbStdout
        $cfp = Get-Ints $stageMPCliffStatusFloorP0P1
        Assert-Condition ($stageMPCliffStatusFloorP0P1.Success -and $cfp[0] -eq 36 -and $cfp[1] -eq 30 -and $cfp[2] -eq 0 -and $cfp[3] -ge 0 -and $cfp[4] -ne 0 -and $cfp[5] -eq 26 -and $cfp[6] -eq 20 -and $cfp[7] -eq 1 -and $cfp[8] -ge 0 -and $cfp[9] -eq 0) 'Stage MP cliff-status final P0/P1 status markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffStatus=ottotto=$($cfp[0])/$($cfp[1]) fall=$($cfp[5])/$($cfp[6]) branches=$($cfs[5])/$($cfs[6]) air=$($cft[3]) lines=$($cfp[3])/$($cfp[8])"
    }
    if ($RequireStageMPCliffTickFloor) {
        $ct = Get-Ints $stageMPCliffTickFloor
        Assert-Condition ($stageMPCliffTickFloor.Success -and $ct[0] -eq 0x46435450 -and $ct[1] -eq 0x46435453 -and (($ct[2] -band 0xfff) -eq 0xfff) -and $ct[3] -eq 0xff -and $ct[4] -eq 2) 'Stage MP cliff-tick floor-loop result/mask did not pass.' $gdbStdout
        $ctc = Get-Ints $stageMPCliffTickFloorCalls
        Assert-Condition ($stageMPCliffTickFloorCalls.Success -and $ctc[0] -eq 1 -and $ctc[1] -eq 1 -and $ctc[2] -eq 1 -and $ctc[3] -eq 1 -and $ctc[4] -eq 1 -and $ctc[5] -eq 1 -and $ctc[6] -eq 1 -and $ctc[7] -eq 1 -and $ctc[8] -eq 1 -and $ctc[9] -eq 1 -and $ctc[10] -eq 1 -and $ctc[11] -eq 1) 'Stage MP cliff-tick callback counts failed.' $gdbStdout
        $cts = Get-Ints $stageMPCliffTickFloorStatus
        Assert-Condition ($stageMPCliffTickFloorStatus.Success -and $cts[0] -eq 36 -and $cts[1] -eq 30 -and $cts[2] -eq 0 -and $cts[3] -eq 36 -and $cts[4] -eq 30 -and $cts[5] -eq 0 -and $cts[6] -ge 0 -and $cts[7] -eq 26 -and $cts[8] -eq 20 -and $cts[9] -eq 1 -and $cts[10] -eq 26 -and $cts[11] -eq 20 -and $cts[12] -eq 1 -and $cts[13] -ge 0 -and $cts[14] -eq 0 -and $cts[15] -eq 0 -and $cts[16] -eq 0) 'Stage MP cliff-tick before/after status markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffTick=ottTick=$($ctc[2])/$($ctc[3])/$($ctc[4]) fallInt=$($ctc[8]) airChecks=$($ctc[9])/$($ctc[10])/$($ctc[11]) lines=$($cts[6])/$($cts[13])"
    }
    if ($RequireStageMPFallMapFloor) {
        $fm = Get-Ints $stageMPFallMapFloor
        Assert-Condition ($stageMPFallMapFloor.Success -and $fm[0] -eq 0x46464d50 -and $fm[1] -eq 0x46464d53 -and (($fm[2] -band 0x3ff) -eq 0x3ff) -and $fm[3] -eq 0xff -and $fm[4] -eq 1) 'Stage MP Fall map floor-loop result/mask did not pass.' $gdbStdout
        $fmc = Get-Ints $stageMPFallMapFloorCalls
        Assert-Condition ($stageMPFallMapFloorCalls.Success -and $fmc[0] -eq 1 -and $fmc[1] -eq 1 -and $fmc[2] -eq 1 -and $fmc[3] -eq 1 -and $fmc[4] -eq 1 -and $fmc[5] -eq 1 -and $fmc[6] -eq 1 -and $fmc[7] -eq 1 -and $fmc[8] -eq 1 -and $fmc[9] -eq 1) 'Stage MP Fall map callback counts failed.' $gdbStdout
        $fms = Get-Ints $stageMPFallMapFloorStatus
        Assert-Condition ($stageMPFallMapFloorStatus.Success -and $fms[0] -eq 26 -and $fms[1] -eq 20 -and $fms[2] -eq 1 -and $fms[3] -eq 26 -and $fms[4] -eq 20 -and $fms[5] -eq 1 -and $fms[6] -ge 0 -and $fms[7] -eq $fms[6] -and $fms[9] -lt $fms[8] -and $fms[11] -lt $fms[10] -and $fms[12] -eq 0) 'Stage MP Fall map before/after status markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpFallMap=phys=$($fmc[2]) fast=$($fmc[3]) grav=$($fmc[4]) map=$($fmc[8]) noColl=$($fmc[9]) y=$($fms[8])->$($fms[9])"
    }
    if ($RequireStageMPFallLandFloor) {
        $fl = Get-Ints $stageMPFallLandFloor
        Assert-Condition ($stageMPFallLandFloor.Success -and $fl[0] -eq 0x464c4e50 -and $fl[1] -eq 0x464c4e53 -and (($fl[2] -band 0x7ff) -eq 0x7ff) -and $fl[3] -eq 0xff -and $fl[4] -eq 1) 'Stage MP Fall landing floor-loop result/mask did not pass.' $gdbStdout
        $flc = Get-Ints $stageMPFallLandFloorCalls
        Assert-Condition ($stageMPFallLandFloorCalls.Success -and $flc[0] -eq 1 -and $flc[1] -eq 1 -and $flc[2] -eq 1 -and $flc[3] -eq 1 -and $flc[4] -eq 1 -and $flc[5] -eq 1 -and $flc[6] -eq 1 -and $flc[7] -eq 1 -and $flc[8] -eq 1 -and $flc[9] -eq 1 -and $flc[10] -eq 1 -and $flc[11] -eq 1 -and $flc[12] -eq 1 -and $flc[13] -le 1 -and $flc[14] -eq 1) 'Stage MP Fall landing callback/status counts failed.' $gdbStdout
        $fls = Get-Ints $stageMPFallLandFloorStatus
        Assert-Condition ($stageMPFallLandFloorStatus.Success -and $fls[0] -eq 26 -and $fls[1] -eq 20 -and $fls[2] -eq 1 -and $fls[3] -eq 31 -and $fls[4] -eq 25 -and $fls[5] -eq 0 -and $fls[6] -ge 0 -and $fls[7] -eq $fls[6] -and $fls[10] -le $fls[8] -and $fls[11] -eq $fls[8] -and $fls[12] -lt 0 -and $fls[13] -lt 0 -and $fls[13] -le $fls[12] -and $fls[14] -eq 1 -and $fls[15] -eq 0 -and $fls[16] -eq 1) 'Stage MP Fall landing before/after status markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpFallLand=phys=$($flc[2]) map=$($flc[8]) floor=$($flc[9]) land=$($flc[12])/$($flc[13]) status=$($fls[0])->$($fls[3])/$($fls[4]) y=$($fls[9])->$($fls[11]) velY=$($fls[12])->$($fls[13])"
    }
    if ($RequireStageMPCeilFloor) {
        $ce = Get-Ints $stageMPCeilFloor
        Assert-Condition ($stageMPCeilFloor.Success -and $ce[0] -eq 0x46434550 -and $ce[1] -eq 0x46434553 -and (($ce[2] -band 0x7f) -eq 0x7f) -and $ce[3] -eq 0xff -and $ce[4] -eq 1) 'Stage MP ceiling floor-loop result/mask did not pass.' $gdbStdout
        $ces = Get-Ints $stageMPCeilFloorSetup
        Assert-Condition ($stageMPCeilFloorSetup.Success -and $ces[0] -eq 1 -and $ces[1] -eq 1 -and $ces[2] -gt 0 -and $ces[3] -ge 0 -and $ces[4] -eq 1 -and $ces[5] -eq 0) 'Stage MP ceiling setup/selected-line markers failed.' $gdbStdout
        $cec = Get-Ints $stageMPCeilFloorCheck
        $ceilCheckOK = $stageMPCeilFloorCheck.Success -and $cec[3] -ge 0 -and $cec[4] -ge 0 -and $cec[5] -ge 0
        if ($RequireStageMPCliffCatchFloor) {
            $ceilCheckOK = $ceilCheckOK -and $cec[0] -ge 1 -and $cec[1] -ge 1 -and $cec[2] -ge 0 -and $cec[6] -ge 1 -and $cec[7] -ge 1 -and $cec[8] -ge 0
        } elseif ($RequireStageMPCeilStatusFloor) {
            $ceilCheckOK = $ceilCheckOK -and $cec[2] -eq 0 -and $cec[8] -eq 0
            $ceilCheckOK = $ceilCheckOK -and $cec[0] -ge 1 -and $cec[1] -ge 1 -and $cec[6] -ge 1 -and $cec[7] -ge 1
        } else {
            $ceilCheckOK = $ceilCheckOK -and $cec[2] -eq 0 -and $cec[8] -eq 0
            $ceilCheckOK = $ceilCheckOK -and $cec[0] -eq 1 -and $cec[1] -eq 1 -and $cec[6] -eq 1 -and $cec[7] -eq 1
        }
        Assert-Condition $ceilCheckOK 'Stage MP ceiling check/sweep markers failed.' $gdbStdout
        $ceq = Get-Ints $stageMPCeilFloorQuery
        $ceilQueryOK = $stageMPCeilFloorQuery.Success -and $ceq[0] -gt 0 -and $ceq[1] -gt 0 -and $ceq[2] -gt 0 -and $ceq[3] -gt 0
        if ($RequireStageMPCeilStatusFloor) {
            $ceilQueryOK = $ceilQueryOK -and $ceq[5] -ge 1 -and $ceq[6] -ge 1 -and $ceq[7] -ge 1
        } else {
            $ceilQueryOK = $ceilQueryOK -and $ceq[5] -eq 1 -and $ceq[6] -eq 1 -and $ceq[7] -eq 1
        }
        Assert-Condition $ceilQueryOK 'Stage MP ceiling query/adjust markers failed.' $gdbStdout
        $cep = Get-Ints $stageMPCeilFloorPos
        Assert-Condition ($stageMPCeilFloorPos.Success -and $cep[0] -gt $cep[2] -and $cep[3] -lt $cep[5] -and $cep[4] -gt $cep[5] -and $cep[6] -lt 0) 'Stage MP ceiling position adjustment markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCeilFloor=line=$($ces[3]) kind=$($ces[4]) check=$($cec[0])/$($cec[1]) diff=$($cec[6])/$($cec[7]) fc=$($ceq[2])/$($ceq[3]) y=$($cep[0])->$($cep[2]) ceil=$($cep[5]) dist=$($cep[6])"
    }
    if ($RequireStageMPCeilStatusFloor) {
        $csf = Get-Ints $stageMPCeilStatusFloor
        Assert-Condition ($stageMPCeilStatusFloor.Success -and $csf[0] -eq 0x46435350 -and $csf[1] -eq 0x46435353 -and (($csf[2] -band 0x1ff) -eq 0x1ff) -and $csf[3] -eq 0xff -and $csf[4] -eq 1) 'Stage MP ceiling-status result/mask did not pass.' $gdbStdout
        $csc = Get-Ints $stageMPCeilStatusFloorCalls
        Assert-Condition ($stageMPCeilStatusFloorCalls.Success -and $csc[0] -eq 1 -and $csc[1] -eq 1 -and $csc[2] -eq 1 -and $csc[3] -eq 1 -and $csc[4] -ge 1 -and $csc[5] -eq 1 -and $csc[6] -eq 1 -and $csc[7] -eq 1 -and $csc[8] -eq 1 -and $csc[9] -eq 0) 'Stage MP ceiling-status callback/collision counts failed.' $gdbStdout
        $css = Get-Ints $stageMPCeilStatusFloorStatus
        Assert-Condition ($stageMPCeilStatusFloorStatus.Success -and $css[0] -ge 0 -and $css[1] -eq 1 -and $css[2] -eq 26 -and $css[3] -eq 20 -and $css[4] -eq 1 -and $css[5] -eq 66 -and $css[6] -eq 57 -and $css[7] -eq 0 -and $css[8] -eq 1 -and $css[9] -eq 1) 'Stage MP ceiling-status before/after status markers failed.' $gdbStdout
        $csp = Get-Ints $stageMPCeilStatusFloorPos
        Assert-Condition ($stageMPCeilStatusFloorPos.Success -and $csp[0] -gt $csp[1] -and $csp[2] -gt 0 -and $csp[3] -eq 0 -and (($csp[4] -band 0x400) -ne 0) -and (($csp[4] -band 0x4000) -ne 0) -and (($csp[5] -band 0x400) -ne 0)) 'Stage MP ceiling-status position/mask markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCeilStatus=line=$($css[0]) status=$($css[2])->$($css[5]) motion=$($css[3])->$($css[6]) ga=$($css[4])->$($css[7]) velY=$($csp[2])->$($csp[3]) masks=0x$('{0:x}' -f $csp[4])/0x$('{0:x}' -f $csp[5])"
    }
    if ($RequireStageMPCliffCatchFloor) {
        $cc = Get-Ints $stageMPCliffCatchFloor
        Assert-Condition ($stageMPCliffCatchFloor.Success -and $cc[0] -eq 0x46434350 -and $cc[1] -eq 0x46434353 -and (($cc[2] -band 0xfff) -eq 0xfff) -and $cc[3] -eq 0xff -and $cc[4] -eq 1) 'Stage MP cliff-catch result/mask did not pass.' $gdbStdout
        $ccc = Get-Ints $stageMPCliffCatchFloorCalls
        Assert-Condition ($stageMPCliffCatchFloorCalls.Success -and $ccc[0] -eq 1 -and $ccc[1] -eq 1 -and $ccc[2] -eq 2 -and $ccc[3] -eq 2 -and $ccc[4] -ge 2 -and $ccc[5] -eq 2 -and $ccc[6] -eq 2 -and $ccc[7] -eq 0 -and $ccc[8] -eq 2 -and $ccc[9] -eq 1 -and $ccc[10] -eq 1 -and $ccc[11] -eq 1 -and $ccc[12] -eq 1 -and $ccc[13] -eq 0) 'Stage MP cliff-catch callback/collision counts failed.' $gdbStdout
        $cce = Get-Ints $stageMPCliffCatchFloorEffects
        Assert-Condition ($stageMPCliffCatchFloorEffects.Success -and $cce[0] -eq 1 -and ($cce[1] -eq 1 -or $cce[1] -eq 2)) 'Stage MP cliff-catch effect/capture markers failed.' $gdbStdout
        $ccs = Get-Ints $stageMPCliffCatchFloorStatus
        Assert-Condition ($stageMPCliffCatchFloorStatus.Success -and $ccs[0] -ge 0 -and $ccs[1] -eq 1 -and $ccs[2] -eq 26 -and $ccs[3] -eq 20 -and $ccs[4] -eq 1 -and $ccs[5] -eq 84 -and $ccs[6] -eq 72 -and $ccs[7] -eq 1 -and $ccs[8] -eq 1 -and $ccs[9] -eq 1 -and $ccs[10] -eq 1 -and $ccs[11] -eq $ccs[0] -and $ccs[12] -eq -1) 'Stage MP cliff-catch before/after status markers failed.' $gdbStdout
        $ccp = Get-Ints $stageMPCliffCatchFloorPos
        Assert-Condition ($stageMPCliffCatchFloorPos.Success -and (($ccp[6] -band 0x2000) -ne 0) -and (($ccp[7] -band 0x2000) -ne 0) -and (($ccp[2] -ne $ccp[4]) -or ($ccp[3] -ne $ccp[5]))) 'Stage MP cliff-catch ledge snap/mask markers failed.' $gdbStdout
        $cco = Get-Ints $stageMPCliffCatchFloorOcc
        Assert-Condition ($stageMPCliffCatchFloorOcc.Success -and $cco[0] -eq 1 -and $cco[1] -eq 1 -and $cco[2] -eq $ccs[0] -and $cco[3] -eq $ccs[0] -and $cco[4] -eq -1 -and $cco[5] -eq -1 -and $cco[6] -eq 26 -and $cco[7] -eq 20 -and $cco[8] -eq 1 -and $cco[9] -eq 0 -and $cco[10] -eq 0 -and $cco[11] -eq 0) 'Stage MP cliff-catch occupancy blocker markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffCatch=line=$($ccs[0]) side=$($ccs[1]) status=$($ccs[2])->$($ccs[5]) motion=$($ccs[3])->$($ccs[6]) occ=$($cco[1])/$($cco[10]) ledge=$($ccp[0])/$($ccp[1]) root=$($ccp[2]),$($ccp[3])->$($ccp[4]),$($ccp[5]) masks=0x$('{0:x}' -f $ccp[6])/0x$('{0:x}' -f $ccp[7])"
    }
    if ($RequireStageMPCliffWaitFloor) {
        $cw = Get-Ints $stageMPCliffWaitFloor
        Assert-Condition ($stageMPCliffWaitFloor.Success -and $cw[0] -eq 0x46435750 -and $cw[1] -eq 0x46435753 -and (($cw[2] -band 0x7ff) -eq 0x7ff) -and $cw[3] -eq 0xff -and $cw[4] -eq 1) 'Stage MP cliff-wait result/mask did not pass.' $gdbStdout
        $cwc = Get-Ints $stageMPCliffWaitFloorCalls
        Assert-Condition ($stageMPCliffWaitFloorCalls.Success -and $cwc[0] -eq 1 -and $cwc[1] -eq 1 -and $cwc[2] -eq 1 -and $cwc[3] -eq 1 -and $cwc[4] -eq 1 -and $cwc[5] -eq 1 -and $cwc[6] -eq 1 -and $cwc[7] -eq 1 -and ($cwc[8] -eq 1 -or $cwc[8] -eq 2) -and $cwc[9] -eq 1 -and $cwc[10] -eq 1 -and $cwc[11] -eq 1 -and $cwc[12] -eq 1 -and $cwc[13] -eq 0) 'Stage MP cliff-wait source-order call counts failed.' $gdbStdout
        $cws = Get-Ints $stageMPCliffWaitFloorStatus
        Assert-Condition ($stageMPCliffWaitFloorStatus.Success -and $cws[0] -eq 84 -and $cws[1] -eq 72 -and $cws[2] -eq 1 -and $cws[3] -eq 85 -and $cws[4] -eq 73 -and $cws[5] -eq 0 -and $cws[6] -eq 85 -and $cws[7] -eq 73 -and $cws[8] -eq 0 -and $cws[9] -eq 1 -and $cws[10] -eq 0 -and $cws[11] -eq $cws[20] -and $cws[12] -eq -1 -and $cws[13] -gt 1 -and $cws[14] -eq $cws[13] -and $cws[15] -eq ($cws[14] - 1) -and $cws[16] -eq 120 -and $cws[17] -ne 0 -and $cws[18] -eq 1 -and $cws[19] -eq 0) 'Stage MP cliff-wait status/fall-wait markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffWait=status=$($cws[0])->$($cws[3])->$($cws[6]) motion=$($cws[1])->$($cws[4])->$($cws[7]) fallWait=$($cws[14])->$($cws[15]) cliff=$($cws[11]) guards=$($cwc[10])/$($cwc[11])/$($cwc[12])"
    }
    if ($RequireStageMPCliffAttackFloor) {
        $ca = Get-Ints $stageMPCliffAttackFloor
        Assert-Condition ($stageMPCliffAttackFloor.Success -and $ca[0] -eq 0x46434150 -and $ca[1] -eq 0x46434153 -and (($ca[2] -band 0x7ff) -eq 0x7ff) -and $ca[3] -eq 0xff -and $ca[4] -eq 1) 'Stage MP cliff-attack result/mask did not pass.' $gdbStdout
        $cac = Get-Ints $stageMPCliffAttackFloorCalls
        Assert-Condition ($stageMPCliffAttackFloorCalls.Success -and $cac[0] -eq 1 -and $cac[1] -eq 1 -and $cac[2] -eq 1 -and $cac[3] -eq 1 -and $cac[4] -eq 0 -and $cac[5] -eq 0 -and $cac[6] -eq 1 -and $cac[7] -eq 1) 'Stage MP cliff-attack interrupt/status call counts failed.' $gdbStdout
        $cas = Get-Ints $stageMPCliffAttackFloorStatus
        Assert-Condition ($stageMPCliffAttackFloorStatus.Success -and $cas[0] -eq 85 -and $cas[1] -eq 73 -and $cas[2] -eq 0 -and $cas[3] -eq 86 -and $cas[4] -eq 74 -and $cas[5] -eq 0 -and $cas[6] -ge 0 -and $cas[7] -eq $cas[6] -and $cas[8] -eq 1 -and $cas[9] -eq $cas[6] -and $cas[10] -eq 1 -and $cas[12] -eq $cas[8] -and $cas[13] -eq $cas[14] -and $cas[14] -ne 0 -and $cas[15] -gt 0 -and $cas[16] -eq $cas[9] -and $cas[17] -eq 0) 'Stage MP cliff-attack status/queue/input markers failed.' $gdbStdout
        $cau = Get-Ints $stageMPCliffAttackFloorSafe
        $cliffAttackPriorFallWaitOK = $cau[3] -gt 0
        Assert-Condition ($stageMPCliffAttackFloorSafe.Success -and $cau[0] -eq 0 -and $cau[1] -ne 0 -and $cau[2] -eq 1 -and $cliffAttackPriorFallWaitOK) 'Stage MP cliff-attack safety markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffAttack=status=$($cas[0])->$($cas[3]) motion=$($cas[1])->$($cas[4]) queue=$($cas[8]) cliff=$($cas[6]) button=0x$('{0:x}' -f $cas[13])"
    }
    if ($RequireStageMPCliffClimbFloor) {
        $clm = Get-Ints $stageMPCliffClimbFloor
        Assert-Condition ($stageMPCliffClimbFloor.Success -and $clm[0] -eq 0x46434c50 -and $clm[1] -eq 0x46434c53 -and (($clm[2] -band 0x7fff) -eq 0x7fff) -and $clm[3] -eq 0xff -and $clm[4] -eq 1) 'Stage MP cliff-climb result/mask did not pass.' $gdbStdout
        $clmc = Get-Ints $stageMPCliffClimbFloorCalls
        Assert-Condition ($stageMPCliffClimbFloorCalls.Success -and $clmc[0] -eq 1 -and $clmc[1] -eq 1 -and $clmc[2] -eq 2 -and $clmc[3] -eq 2 -and $clmc[4] -eq 2 -and $clmc[5] -eq 2 -and $clmc[6] -eq 1 -and $clmc[7] -eq 1 -and $clmc[8] -eq 1 -and $clmc[9] -eq 0) 'Stage MP cliff-climb source-order call counts failed.' $gdbStdout
        $clmb = Get-Ints $stageMPCliffClimbFloorBase
        Assert-Condition ($stageMPCliffClimbFloorBase.Success -and $clmb[0] -eq 85 -and $clmb[1] -eq 73 -and $clmb[2] -eq 0 -and $clmb[3] -ge 0 -and $clmb[4] -eq -1 -and $clmb[5] -gt 1 -and $clmb[6] -eq 1) 'Stage MP cliff-climb base CliffWait markers failed.' $gdbStdout
        $clmcl = Get-Ints $stageMPCliffClimbFloorClimb
        Assert-Condition ($stageMPCliffClimbFloorClimb.Success -and $clmcl[1] -gt 20 -and $clmcl[2] -eq 86 -and $clmcl[3] -eq 74 -and $clmcl[4] -eq 0 -and $clmcl[5] -eq $clmb[3] -and $clmcl[6] -eq 0 -and $clmcl[7] -eq $clmb[3] -and $clmcl[8] -eq 1 -and $clmcl[9] -eq 1 -and $clmcl[10] -eq $clmcl[6] -and $clmcl[11] -eq $clmcl[7]) 'Stage MP cliff-climb Quick branch markers failed.' $gdbStdout
        $clmd = Get-Ints $stageMPCliffClimbFloorDrop
        Assert-Condition ($stageMPCliffClimbFloorDrop.Success -and $clmd[0] -gt 20 -and $clmd[2] -eq 26 -and $clmd[3] -eq 20 -and $clmd[4] -eq 1 -and $clmd[5] -eq $clmb[3] -and $clmd[6] -eq $clmb[5] -and $clmd[7] -eq 30 -and $clmd[8] -eq 0 -and $clmd[9] -eq 0 -and $clmd[10] -eq 1 -and $clmd[11] -eq 0) 'Stage MP cliff-climb Fall branch markers failed.' $gdbStdout
        $clmr = Get-Ints $stageMPCliffClimbFloorRecatch
        Assert-Condition ($stageMPCliffClimbFloorRecatch.Success -and $clmr[0] -eq 1 -and $clmr[1] -eq 1 -and $clmr[2] -eq 1 -and $clmr[3] -ge 1 -and $clmr[4] -eq 1 -and $clmr[5] -eq 0 -and $clmr[6] -eq 1 -and $clmr[7] -eq 1 -and $clmr[8] -eq 1 -and $clmr[9] -eq 1 -and $clmr[10] -eq 1 -and $clmr[11] -eq 0) 'Stage MP cliff-climb recatch source-order counts failed.' $gdbStdout
        $clmrs = Get-Ints $stageMPCliffClimbFloorRecatchStatus
        Assert-Condition ($stageMPCliffClimbFloorRecatchStatus.Success -and $clmrs[0] -eq 0 -and $clmrs[1] -eq 26 -and $clmrs[2] -eq 20 -and $clmrs[3] -eq 1 -and $clmrs[4] -eq $clmb[3] -and $clmrs[5] -eq 26 -and $clmrs[6] -eq 20 -and $clmrs[7] -eq 1 -and $clmrs[8] -eq 84 -and $clmrs[9] -eq 72 -and $clmrs[10] -eq 1 -and $clmrs[11] -eq 1 -and $clmrs[12] -eq $clmb[3] -and $clmrs[13] -eq -1 -and (($clmrs[14] -band 0x2000) -ne 0) -and (($clmrs[15] -band 0x2000) -ne 0)) 'Stage MP cliff-climb recatch status/occupancy markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffClimb=climbStatus=$($clmcl[2])/$($clmcl[3]) dropStatus=$($clmd[2])/$($clmd[3]) cliff=$($clmb[3]) sticks=$($clmcl[0]),$($clmcl[1])/$($clmd[0]),$($clmd[1]) calls=$($clmc[2])/$($clmc[5]) recatch=$($clmrs[5])->$($clmrs[8]) hold=$($clmrs[0])/$($clmrs[11]) block=$($clmr[11])"
    }
    if ($RequireStageMPCliffClimbAction) {
        $cla = Get-Ints $stageMPCliffClimbAction
        Assert-Condition ($stageMPCliffClimbAction.Success -and $cla[0] -eq 0x46434c41 -and $cla[1] -eq 0x46434c55 -and (($cla[2] -band 0xfff) -eq 0xfff) -and $cla[3] -eq 0xff -and $cla[4] -eq 1) 'Stage MP cliff-climb action result/mask did not pass.' $gdbStdout
        $clac = Get-Ints $stageMPCliffClimbActionCalls
        Assert-Condition ($stageMPCliffClimbActionCalls.Success -and $clac[0] -eq 1 -and $clac[1] -eq 1 -and $clac[2] -eq 1 -and $clac[3] -eq 1 -and $clac[4] -eq 1 -and $clac[5] -eq 1 -and $clac[6] -eq 1 -and $clac[7] -eq 1 -and $clac[8] -eq 1 -and $clac[9] -eq 0) 'Stage MP cliff-climb action source-order call counts failed.' $gdbStdout
        $clas = Get-Ints $stageMPCliffClimbActionStatus
        Assert-Condition ($stageMPCliffClimbActionStatus.Success -and $clas[0] -eq 86 -and $clas[1] -eq 74 -and $clas[2] -eq 0 -and $clas[3] -eq 87 -and $clas[4] -eq 75 -and $clas[5] -eq 0 -and $clas[6] -eq 88 -and $clas[7] -eq 76 -and $clas[8] -eq 0 -and $clas[9] -ge 0 -and $clas[10] -eq $clas[9] -and $clas[11] -eq $clas[9] -and $clas[12] -eq $clas[9] -and $clas[13] -eq 0 -and $clas[14] -eq $clas[9]) 'Stage MP cliff-climb action status/ledge/floor-copyback markers failed.' $gdbStdout
        $claf = Get-Ints $stageMPCliffClimbActionFlags
        Assert-Condition ($stageMPCliffClimbActionFlags.Success -and $claf[0] -eq 1 -and $claf[1] -eq 0 -and $claf[2] -eq 1 -and $claf[3] -eq 0 -and $claf[4] -eq 1 -and $claf[5] -eq 1 -and $claf[6] -eq 1) 'Stage MP cliff-climb action callback/flag markers failed.' $gdbStdout
        if (-not $RequireStageMPCliffAttackAction) {
            Assert-CliffCommon2BridgeRoot $stageMPCliffClimbActionRoot 0 'Stage MP cliff-climb action common2 bridge/root markers failed.' $gdbStdout
        }
        $clar = Get-Ints $stageMPCliffClimbActionRoot
        $stageSummary = "$stageSummary mpCliffClimbAction=status=$($clas[0])->$($clas[3])->$($clas[6]) motion=$($clas[1])->$($clas[4])->$($clas[7]) cliff=$($clas[9]) floor=$($clas[12]) calls=$($clac[2])/$($clac[4])/$($clac[6])"
        $stageSummary = "$stageSummary climbBridge=root=$($clar[6]),$($clar[7])->$($clar[8]),$($clar[9]) exp=$($clar[10]),$($clar[11])"
    }
    if ($RequireStageMPCliffClimbCommon2) {
        $cl2 = Get-Ints $stageMPCliffClimbCommon2
        Assert-Condition ($stageMPCliffClimbCommon2.Success -and $cl2[0] -eq 0x464c3250 -and $cl2[1] -eq 0x464c3253 -and (($cl2[2] -band 0x1ff) -eq 0x1ff) -and $cl2[3] -eq 0xff -and $cl2[4] -eq 1) 'Stage MP cliff-climb common2 result/mask did not pass.' $gdbStdout
        $cl2c = Get-Ints $stageMPCliffClimbCommon2Calls
        Assert-Condition ($stageMPCliffClimbCommon2Calls.Success -and $cl2c[0] -eq 1 -and $cl2c[1] -eq 1 -and $cl2c[2] -eq 1 -and $cl2c[3] -eq 1 -and $cl2c[4] -eq 0 -and $cl2c[5] -eq 1 -and $cl2c[6] -eq 1 -and $cl2c[7] -eq 1 -and $cl2c[8] -eq 1 -and $cl2c[9] -eq 0) 'Stage MP cliff-climb common2 callback/seam counts failed.' $gdbStdout
        $cl2s = Get-Ints $stageMPCliffClimbCommon2Status
        Assert-Condition ($stageMPCliffClimbCommon2Status.Success -and $cl2s[0] -eq 88 -and $cl2s[1] -eq 76 -and $cl2s[2] -eq 0 -and $cl2s[3] -eq 88 -and $cl2s[4] -eq 76 -and $cl2s[5] -eq 0 -and $cl2s[6] -eq 88 -and $cl2s[7] -eq 76 -and $cl2s[8] -eq 0 -and $cl2s[9] -eq 88 -and $cl2s[10] -eq 76 -and $cl2s[11] -eq 0) 'Stage MP cliff-climb common2 before/after status markers failed.' $gdbStdout
        $cl2f = Get-Ints $stageMPCliffClimbCommon2Flags
        Assert-Condition ($stageMPCliffClimbCommon2Flags.Success -and $cl2f[0] -ge 0 -and $cl2f[1] -eq $cl2f[0] -and $cl2f[2] -eq $cl2f[0] -and $cl2f[3] -eq $cl2f[1] -and $cl2f[4] -eq 1 -and $cl2f[5] -eq 1 -and $cl2f[6] -eq 1 -and $cl2f[7] -eq 0) 'Stage MP cliff-climb common2 floor/proc markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffClimbCommon2=status=$($cl2s[0])->$($cl2s[3])->$($cl2s[6])->$($cl2s[9]) motion=$($cl2s[1])->$($cl2s[4])->$($cl2s[7])->$($cl2s[10]) cliff=$($cl2f[0]) floor=$($cl2f[1])->$($cl2f[3]) calls=$($cl2c[2])/$($cl2c[5])/$($cl2c[7])"
    }
    if ($RequireStageMPCliffClimbFinish) {
        $clf = Get-Ints $stageMPCliffClimbFinish
        Assert-Condition ($stageMPCliffClimbFinish.Success -and $clf[0] -eq 0x46434650 -and $clf[1] -eq 0x46434653 -and (($clf[2] -band 0x3ff) -eq 0x3ff) -and $clf[3] -eq 0xff -and $clf[4] -eq 1) 'Stage MP cliff-climb finish result/mask did not pass.' $gdbStdout
        $clfc = Get-Ints $stageMPCliffClimbFinishCalls
        Assert-Condition ($stageMPCliffClimbFinishCalls.Success -and $clfc[0] -eq 1 -and $clfc[1] -eq 1 -and $clfc[2] -eq 1 -and $clfc[3] -eq 1 -and $clfc[4] -eq 1 -and $clfc[5] -eq 1 -and $clfc[6] -eq 1 -and $clfc[7] -eq 0) 'Stage MP cliff-climb finish callback/seam counts failed.' $gdbStdout
        $clfs = Get-Ints $stageMPCliffClimbFinishStatus
        Assert-Condition ($stageMPCliffClimbFinishStatus.Success -and $clfs[0] -eq 88 -and $clfs[1] -eq 76 -and $clfs[2] -eq 0 -and $clfs[3] -eq 10 -and $clfs[4] -eq 4 -and $clfs[5] -eq 0) 'Stage MP cliff-climb finish status markers failed.' $gdbStdout
        $clff = Get-Ints $stageMPCliffClimbFinishFlags
        Assert-Condition ($stageMPCliffClimbFinishFlags.Success -and $clff[0] -ge 0 -and $clff[1] -eq $clff[0] -and $clff[2] -eq $clff[0] -and $clff[3] -eq $clff[1] -and $clff[4] -eq 0 -and $clff[5] -eq 1 -and $clff[6] -eq 1 -and $clff[7] -eq 1 -and $clff[8] -eq 120 -and $clff[9] -eq 0 -and (($clff[10] -band 0x7ffff) -eq 0x7ffff)) 'Stage MP cliff-climb finish floor/proc/player-tag/common-reset markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffClimbFinish=status=$($clfs[0])->$($clfs[3]) motion=$($clfs[1])->$($clfs[4]) cliff=$($clff[0]) floor=$($clff[1])->$($clff[3]) reset=$($clff[4])/$($clff[9])/0x$('{0:x}' -f $clff[10]) calls=$($clfc[2])/$($clfc[4])/$($clfc[5])"
    }
    if ($RequireStageMPCliffWaitDamage) {
        $cwd = Get-Ints $stageMPCliffWaitDamage
        Assert-Condition ($stageMPCliffWaitDamage.Success -and $cwd[0] -eq 0x46445750 -and $cwd[1] -eq 0x46445753 -and (($cwd[2] -band 0x1ffffff) -eq 0x1ffffff) -and $cwd[3] -eq 0xff -and $cwd[4] -eq 1) 'Stage MP cliff-wait damage result/mask did not pass.' $gdbStdout
        $cwdc = Get-Ints $stageMPCliffWaitDamageCalls
        Assert-Condition ($stageMPCliffWaitDamageCalls.Success -and $cwdc[0] -eq 1 -and $cwdc[1] -eq 1 -and $cwdc[2] -eq 1 -and $cwdc[3] -eq 1 -and $cwdc[4] -eq 1 -and $cwdc[5] -eq 1 -and $cwdc[6] -eq 1 -and $cwdc[7] -eq 1 -and $cwdc[8] -eq 1 -and $cwdc[9] -eq 1 -and $cwdc[10] -eq 0) 'Stage MP cliff-wait damage source-order call counts failed.' $gdbStdout
        $cwds = Get-Ints $stageMPCliffWaitDamageStatus
        Assert-Condition ($stageMPCliffWaitDamageStatus.Success -and $cwds[0] -eq 85 -and $cwds[1] -eq 73 -and $cwds[2] -eq 0 -and $cwds[3] -eq 57 -and $cwds[4] -eq 50 -and $cwds[5] -eq 1) 'Stage MP cliff-wait damage status markers failed.' $gdbStdout
        $cwdf = Get-Ints $stageMPCliffWaitDamageFlags
        Assert-Condition ($stageMPCliffWaitDamageFlags.Success -and $cwdf[0] -eq 1 -and $cwdf[1] -eq 0 -and $cwdf[2] -eq 30 -and $cwdf[3] -ge 0 -and $cwdf[4] -eq $cwdf[3] -and $cwdf[5] -eq $cwdf[3] -and $cwdf[6] -eq 1 -and $cwdf[7] -eq 0 -and $cwdf[8] -eq 0 -and $cwdf[9] -eq 65536 -and $cwdf[10] -eq 1) 'Stage MP cliff-wait damage flag/callback markers failed.' $gdbStdout
        $cwdp = Get-Ints $stageMPCliffWaitDamagePos
        Assert-Condition ($stageMPCliffWaitDamagePos.Success -and $cwdp[2] -eq $cwdp[4] -and $cwdp[3] -eq $cwdp[5] -and (($cwdp[0] -ne $cwdp[4]) -or ($cwdp[1] -ne $cwdp[5]))) 'Stage MP cliff-wait damage placement markers failed.' $gdbStdout
        $cwdt = Get-Ints $stageMPCliffWaitDamageTick
        Assert-Condition ($stageMPCliffWaitDamageTick.Success -and $cwdt[0] -eq 1 -and $cwdt[1] -eq 1 -and $cwdt[2] -eq 1 -and $cwdt[3] -eq 1 -and $cwdt[4] -eq 1 -and $cwdt[5] -eq 1 -and $cwdt[6] -eq 5 -and $cwdt[7] -eq 5 -and $cwdt[8] -eq 1 -and $cwdt[9] -eq 3 -and $cwdt[10] -eq 2 -and $cwdt[11] -eq 1 -and $cwdt[13] -lt $cwdt[12] -and $cwdt[14] -eq 57 -and $cwdt[15] -eq 50 -and $cwdt[16] -eq 1) 'Stage MP cliff-wait DamageFall tick markers failed.' $gdbStdout
        $cwdx = Get-Ints $stageMPCliffWaitDamageCollision
        Assert-Condition ($stageMPCliffWaitDamageCollision.Success -and $cwdx[0] -eq 4 -and $cwdx[1] -eq 1 -and $cwdx[2] -eq 1 -and $cwdx[3] -eq 1 -and $cwdx[4] -eq 1 -and $cwdx[5] -eq 1 -and $cwdx[6] -eq 1 -and $cwdx[7] -eq 68 -and $cwdx[8] -eq 59 -and $cwdx[9] -eq 0 -and $cwdx[10] -eq 0 -and $cwdx[11] -eq 500 -and $cwdx[12] -eq 1 -and $cwdx[13] -eq 22 -and $cwdx[14] -ne 0 -and $cwdx[15] -eq 4) 'Stage MP cliff-wait DamageFall collision/DownBounce markers failed.' $gdbStdout
        $cwdcc = Get-Ints $stageMPCliffWaitDamageCliffCatch
        Assert-Condition ($stageMPCliffWaitDamageCliffCatch.Success -and $cwdcc[0] -eq 1 -and $cwdcc[1] -eq 1 -and $cwdcc[2] -eq 1 -and $cwdcc[3] -eq 1 -and $cwdcc[4] -eq 1 -and $cwdcc[5] -eq 1 -and $cwdcc[6] -eq 1 -and ($cwdcc[7] -eq 1 -or $cwdcc[7] -eq 2) -and $cwdcc[8] -eq 84 -and $cwdcc[9] -eq 72 -and $cwdcc[10] -eq 1 -and $cwdcc[11] -eq 1 -and $cwdcc[12] -eq 1 -and $cwdcc[13] -eq 1 -and $cwdcc[14] -eq $cwdf[3] -and $cwdcc[15] -eq -1 -and (($cwdcc[16] -band 0x2000) -ne 0) -and (($cwdcc[17] -band 0x2000) -ne 0) -and $cwdcc[18] -eq 0x4) 'Stage MP cliff-wait DamageFall CliffCatch markers failed.' $gdbStdout
        $cwdps = Get-Ints $stageMPCliffWaitDamagePassiveStand
        Assert-Condition ($stageMPCliffWaitDamagePassiveStand.Success -and $cwdps[0] -eq 1 -and $cwdps[1] -eq 1 -and $cwdps[2] -eq 1 -and $cwdps[3] -eq 73 -and $cwdps[4] -eq 62 -and $cwdps[5] -eq 0 -and $cwdps[6] -le -20 -and $cwdps[7] -eq -1 -and $cwdps[8] -eq 1) 'Stage MP cliff-wait PassiveStand markers failed.' $gdbStdout
        $cwdpvs = Get-Ints $stageMPCliffWaitDamagePassive
        Assert-Condition ($stageMPCliffWaitDamagePassive.Success -and $cwdpvs[0] -eq 1 -and $cwdpvs[1] -eq 1 -and $cwdpvs[2] -eq 1 -and $cwdpvs[3] -eq 81 -and $cwdpvs[4] -eq 70 -and $cwdpvs[5] -eq 0 -and $cwdpvs[6] -eq 1) 'Stage MP cliff-wait Passive markers failed.' $gdbStdout
        $cwdpsc = Get-Ints $stageMPCliffWaitDamagePassiveStandCallbacks
        Assert-Condition ($stageMPCliffWaitDamagePassiveStandCallbacks.Success -and $cwdpsc[0] -eq 1 -and $cwdpsc[1] -eq 1 -and $cwdpsc[2] -eq 73 -and $cwdpsc[3] -eq 62 -and $cwdpsc[4] -eq 0 -and $cwdpsc[5] -eq 1) 'Stage MP cliff-wait PassiveStand callback markers failed.' $gdbStdout
        $cwdpvc = Get-Ints $stageMPCliffWaitDamagePassiveCallbacks
        Assert-Condition ($stageMPCliffWaitDamagePassiveCallbacks.Success -and $cwdpvc[0] -eq 1 -and $cwdpvc[1] -eq 1 -and $cwdpvc[2] -eq 81 -and $cwdpvc[3] -eq 70 -and $cwdpvc[4] -eq 0 -and $cwdpvc[5] -eq 1) 'Stage MP cliff-wait Passive callback markers failed.' $gdbStdout
        $cwdpst = Get-Ints $stageMPCliffWaitDamagePassiveStandTick
        Assert-Condition ($stageMPCliffWaitDamagePassiveStandTick.Success -and $cwdpst[0] -eq 1 -and $cwdpst[1] -eq 1 -and $cwdpst[2] -eq 1 -and $cwdpst[3] -eq 10 -and $cwdpst[4] -eq 4 -and $cwdpst[5] -eq 0 -and $cwdpst[6] -eq 120 -and $cwdpst[7] -eq 1) 'Stage MP cliff-wait PassiveStand update-to-Wait markers failed.' $gdbStdout
        $cwdpvt = Get-Ints $stageMPCliffWaitDamagePassiveTick
        Assert-Condition ($stageMPCliffWaitDamagePassiveTick.Success -and $cwdpvt[0] -eq 1 -and $cwdpvt[1] -eq 1 -and $cwdpvt[2] -eq 1 -and $cwdpvt[3] -eq 10 -and $cwdpvt[4] -eq 4 -and $cwdpvt[5] -eq 0 -and $cwdpvt[6] -eq 120 -and $cwdpvt[7] -eq 1) 'Stage MP cliff-wait Passive update-to-Wait markers failed.' $gdbStdout
        $cwdb = Get-Ints $stageMPCliffWaitDamageDownBounce
        Assert-Condition ($stageMPCliffWaitDamageDownBounce.Success -and $cwdb[0] -eq 2 -and $cwdb[1] -eq 1 -and $cwdb[2] -eq 1 -and $cwdb[3] -eq 60 -and $cwdb[4] -eq 68 -and $cwdb[5] -eq 59 -and $cwdb[6] -eq 0) 'Stage MP cliff-wait DownBounce update markers failed.' $gdbStdout
        $cwdw = Get-Ints $stageMPCliffWaitDamageDownWait
        Assert-Condition ($stageMPCliffWaitDamageDownWait.Success -and $cwdw[0] -eq 1 -and ($cwdw[1] -eq 1 -or $cwdw[1] -eq 2) -and $cwdw[2] -eq 70 -and $cwdw[3] -eq -2 -and $cwdw[4] -eq 0 -and $cwdw[5] -eq 180 -and $cwdw[6] -eq 0x33 -and $cwdw[7] -eq 500 -and $cwdw[8] -eq 1 -and $cwdw[9] -eq 2 -and $cwdw[10] -eq 180 -and $cwdw[11] -eq 179 -and $cwdw[12] -eq 0 -and $cwdw[13] -eq 70 -and $cwdw[14] -eq -2 -and $cwdw[15] -eq 0) 'Stage MP cliff-wait DownWait setup/update markers failed.' $gdbStdout
        $cwdsd = Get-Ints $stageMPCliffWaitDamageDownStand
        Assert-Condition ($stageMPCliffWaitDamageDownStand.Success -and $cwdsd[0] -eq 1 -and $cwdsd[1] -eq 1 -and $cwdsd[2] -eq 72 -and $cwdsd[3] -eq 61 -and $cwdsd[4] -eq 0 -and $cwdsd[5] -eq 1 -and $cwdsd[6] -eq 0 -and $cwdsd[7] -eq 1 -and $cwdsd[8] -eq 0 -and $cwdsd[9] -eq 1000) 'Stage MP cliff-wait DownStand timeout markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffWaitDamage=status=$($cwds[0])->$($cwds[3]) motion=$($cwds[1])->$($cwds[4]) fallWait=$($cwdf[0])->$($cwdf[1]) catch=$($cwdf[2]) tics=$($cwdf[9]) hold=$($cwdf[6])->$($cwdf[7]) procDmg=$($cwdf[8]) dmgTick=$($cwdt[0])/$($cwdt[5])/$($cwdt[6]) velY=$($cwdt[12])->$($cwdt[13]) cliffCatch=$($cwdcc[8])/$($cwdcc[9])/$($cwdcc[10]) hold=$($cwdcc[11]) passiveStand=$($cwdps[3])/$($cwdps[4])/$($cwdps[5])->cb$($cwdpsc[0])/$($cwdpsc[1])->$($cwdpst[3])/$($cwdpst[4])/$($cwdpst[5]) passive=$($cwdpvs[3])/$($cwdpvs[4])/$($cwdpvs[5])->cb$($cwdpvc[0])/$($cwdpvc[1])->$($cwdpvt[3])/$($cwdpvt[4])/$($cwdpvt[5]) downBounce=$($cwdx[7])/$($cwdx[8])/$($cwdx[9]) dbuf=$($cwdb[3]) downWait=$($cwdw[2])/$($cwdw[3]) wait=$($cwdw[10])->$($cwdw[11]) downStand=$($cwdsd[2])/$($cwdsd[3]) flag1=$($cwdsd[5])->$($cwdsd[6]) dmgMul=$($cwdsd[9]) coll=$($cwdx[0])"
    }
    if ($RequireStageMPPassiveLoop) {
        $passiveStableFrameExpected = if ($RequireStageMPPassiveRecoverLoop) { 5 } else { 2 }
        $passiveUpdateTickExpected = $passiveStableFrameExpected + 1
        $passiveMaskExpected = if ($RequireStageMPPassiveRecoverLoop) { 0x1fffff } else { 0x1ff }
        $pl = Get-Ints $stageMPPassiveLoop
        Assert-Condition ($stageMPPassiveLoop.Success -and $pl[0] -eq 0x46505350 -and $pl[1] -eq 0x46505353 -and (($pl[2] -band $passiveMaskExpected) -eq $passiveMaskExpected) -and $pl[3] -eq 0xff -and $pl[4] -eq 1) 'Stage MP Passive-loop result/mask did not pass.' $gdbStdout
        if ($RequireStageMPPassiveRecoverLoop) {
            $pbr = Get-Ints $stageMPPassiveLoopBranch
            Assert-Condition ($stageMPPassiveLoopBranch.Success -and (($pbr[0] -band 0x7f) -eq 0x7f)) 'Stage MP Passive recover branch matrix did not pass.' $gdbStdout
            $psb = Get-Ints $stageMPPassiveLoopPassiveStandB
            Assert-Condition ($stageMPPassiveLoopPassiveStandB.Success -and (($psb[0] -band 0xf) -eq 0xf)) 'Stage MP PassiveStandB recover handoff did not pass.' $gdbStdout
            $pnm = Get-Ints $stageMPPassiveLoopNaturalMap
            Assert-Condition ($stageMPPassiveLoopNaturalMap.Success -and (($pnm[0] -band 0x7f) -eq 0x7f)) 'Stage MP Passive natural DamageFall map path did not pass.' $gdbStdout
            Assert-Condition ($stageMPPassiveLoopNaturalMapCalls.Success -and [int]$stageMPPassiveLoopNaturalMapCalls.Groups[1].Value -eq 2) 'Stage MP Passive natural DamageFall map call count did not pass.' $gdbStdout
            $pa = Get-Ints $stageMPPassiveLoopAppeal
            Assert-Condition ($stageMPPassiveLoopAppeal.Success -and (($pa[0] -band 0x7f) -eq 0x7f) -and $pa[1] -eq 2 -and $pa[2] -eq 2 -and $pa[3] -eq 189 -and $pa[4] -eq 164 -and $pa[5] -eq 0 -and $pa[6] -eq 1 -and $pa[7] -ne 0 -and $pa[8] -ne 0) 'Stage MP Passive Appeal branch did not pass.' $gdbStdout
            $pag = Get-Ints $stageMPPassiveLoopAppealGuard
            Assert-Condition ($stageMPPassiveLoopAppealGuard.Success -and (($pag[0] -band 0x7f) -eq 0x7f) -and $pag[1] -eq 1 -and $pag[2] -eq 1 -and $pag[3] -eq 1 -and $pag[4] -eq 152 -and $pag[5] -eq 134 -and $pag[6] -eq 1 -and $pag[7] -eq 1 -and $pag[8] -ne 0 -and $pag[9] -ne 0) 'Stage MP Passive Appeal GuardOn branch did not pass.' $gdbStdout
            $pc = Get-Ints $stageMPPassiveLoopCatch
            Assert-Condition ($stageMPPassiveLoopCatch.Success -and (($pc[0] -band 0x1fff) -eq 0x1fff) -and $pc[1] -eq 1 -and $pc[2] -eq 1 -and $pc[3] -eq 1 -and $pc[4] -eq 166 -and $pc[5] -eq 146 -and $pc[6] -eq 0 -and $pc[7] -eq 1 -and $pc[8] -eq 1 -and $pc[9] -eq 1 -and $pc[10] -eq 0 -and $pc[11] -eq 0 -and $pc[12] -eq 0) 'Stage MP Passive Catch branch did not pass.' $gdbStdout
            $pcc = Get-Ints $stageMPPassiveLoopCatchCallbacks
            Assert-Condition ($stageMPPassiveLoopCatchCallbacks.Success -and (($pcc[0] -band 0x7f) -eq 0x7f) -and $pcc[1] -eq 1 -and $pcc[2] -eq 1 -and $pcc[3] -eq 1 -and $pcc[4] -eq 166 -and $pcc[5] -eq 146 -and $pcc[6] -eq 0 -and $pcc[7] -eq 10 -and $pcc[8] -eq 4 -and $pcc[9] -eq 0) 'Stage MP Passive Catch callback proof did not pass.' $gdbStdout
            $pcp = Get-Ints $stageMPPassiveLoopCatchPull
            Assert-Condition ($stageMPPassiveLoopCatchPull.Success -and (($pcp[0] -band 0xff) -eq 0xff) -and $pcp[1] -eq 1 -and $pcp[2] -eq 1 -and $pcp[3] -eq 1 -and $pcp[4] -eq 1 -and $pcp[5] -eq 1 -and $pcp[6] -eq 1 -and (($pcp[7] -eq 2) -or ($pcp[7] -eq 4)) -and $pcp[8] -eq 1 -and $pcp[9] -eq 1 -and $pcp[10] -eq 167 -and $pcp[11] -eq 147 -and $pcp[12] -eq 0 -and $pcp[13] -eq 168 -and $pcp[14] -eq 4294967294 -and $pcp[15] -eq 0 -and $pcp[16] -eq 60 -and $pcp[17] -eq 59 -and $pcp[18] -eq 1 -and $pcp[19] -eq 1 -and $pcp[20] -eq 1 -and $pcp[21] -eq 0) 'Stage MP Passive CatchPull/CatchWait proof did not pass.' $gdbStdout
            $pcap = Get-Ints $stageMPPassiveLoopCapture
            Assert-Condition ($stageMPPassiveLoopCapture.Success -and (($pcap[0] -band 0x3f) -eq 0x3f) -and $pcap[1] -eq 1 -and $pcap[2] -eq 1 -and $pcap[3] -eq 1 -and $pcap[4] -eq 1 -and $pcap[5] -eq 1 -and $pcap[6] -eq 1 -and $pcap[7] -eq 1 -and $pcap[8] -eq 1 -and (($pcap[9] -eq 2) -or ($pcap[9] -eq 4)) -and $pcap[10] -eq 171 -and $pcap[11] -eq 150 -and $pcap[12] -eq 0 -and $pcap[13] -eq 172 -and $pcap[14] -eq 4294967294 -and $pcap[15] -eq 0 -and $pcap[16] -eq 1 -and ([Math]::Abs($pcap[17]) -eq 1) -and $pcap[18] -eq 0 -and $pcap[21] -eq 0) 'Stage MP Passive CapturePulled/CaptureWait proof did not pass.' $gdbStdout
            $pthr = Get-Ints $stageMPPassiveLoopThrow
            Assert-Condition ($stageMPPassiveLoopThrow.Success -and (($pthr[0] -band 0x3ff) -eq 0x3ff) -and $pthr[1] -eq 2 -and $pthr[2] -eq 2 -and $pthr[3] -eq 2 -and $pthr[4] -eq 4 -and (($pthr[5] -eq 4) -or ($pthr[5] -eq 8)) -and $pthr[6] -eq 169 -and $pthr[7] -eq 148 -and $pthr[8] -eq 0 -and $pthr[9] -eq 1 -and $pthr[10] -eq 186 -and $pthr[11] -eq 161 -and $pthr[12] -eq 1 -and $pthr[13] -eq 1 -and $pthr[14] -eq 1 -and $pthr[15] -eq 0) 'Stage MP Passive ThrowF/ThrowB/ThrownCommon proof did not pass.' $gdbStdout
            $pthrb = Get-Ints $stageMPPassiveLoopThrowB
            Assert-Condition ($stageMPPassiveLoopThrowB.Success -and $pthrb[0] -eq 1 -and $pthrb[1] -eq 170 -and $pthrb[2] -eq 149 -and $pthrb[3] -eq 0 -and $pthrb[4] -eq 1 -and $pthrb[5] -eq 186 -and $pthrb[6] -eq 161 -and $pthrb[7] -eq 1 -and $pthrb[8] -eq 1 -and $pthrb[9] -eq 1) 'Stage MP Passive ThrowB branch proof did not pass.' $gdbStdout
            $pthrc = Get-Ints $stageMPPassiveLoopThrowCallback
            Assert-Condition ($stageMPPassiveLoopThrowCallback.Success -and (($pthrc[0] -band 0x1ff) -eq 0x1ff) -and $pthrc[1] -eq 1 -and $pthrc[2] -eq 1 -and $pthrc[3] -eq 1 -and $pthrc[4] -eq 186 -and $pthrc[5] -eq 161 -and $pthrc[6] -eq 1 -and $pthrc[7] -ge 0 -and $pthrc[8] -eq 1 -and $pthrc[9] -eq 1 -and $pthrc[10] -eq 1 -and $pthrc[11] -eq 1 -and (($pthrc[12] -eq 1) -or ($pthrc[12] -eq 2)) -and $pthrc[13] -eq 186 -and $pthrc[14] -eq 161 -and $pthrc[15] -eq 1 -and $pthrc[16] -eq 1 -and $pthrc[17] -eq 1) 'Stage MP Passive ThrownCommon callback proof did not pass.' $gdbStdout
            $pthru = Get-Ints $stageMPPassiveLoopThrowUpdate
            Assert-Condition ($stageMPPassiveLoopThrowUpdate.Success -and (($pthru[0] -band 0x7f) -eq 0x7f) -and $pthru[1] -eq 1 -and $pthru[2] -eq 1 -and $pthru[3] -eq 169 -and $pthru[4] -eq 148 -and $pthru[5] -eq 0 -and $pthru[6] -eq 1 -and $pthru[7] -eq 0 -and $pthru[8] -eq 0 -and $pthru[9] -eq 50 -and $pthru[10] -eq 58 -and $pthru[11] -eq 1 -and $pthru[12] -eq 1 -and $pthru[13] -eq 1 -and $pthru[14] -eq 0 -and $pthru[15] -eq -1) 'Stage MP Passive Throw update release proof did not pass.' $gdbStdout
            $pthrr = Get-Ints $stageMPPassiveLoopThrowRelease
            Assert-Condition ($stageMPPassiveLoopThrowRelease.Success -and (($pthrr[0] -band 0xff) -eq 0xff) -and $pthrr[1] -eq 1 -and $pthrr[2] -eq 1 -and $pthrr[3] -eq 1 -and $pthrr[4] -eq 1 -and $pthrr[5] -eq 1 -and $pthrr[6] -eq 1 -and $pthrr[7] -eq 3 -and $pthrr[8] -eq 1 -and $pthrr[9] -eq 1 -and $pthrr[10] -eq 1 -and $pthrr[11] -eq 0 -and $pthrr[12] -eq 10 -and $pthrr[13] -eq 18 -and $pthrr[14] -eq 8 -and $pthrr[15] -gt 0 -and ([Math]::Abs($pthrr[16]) -eq 1) -and $pthrr[17] -eq 123) 'Stage MP Passive Throw release/damage proof did not pass.' $gdbStdout
            $pthrs = Get-Ints $stageMPPassiveLoopThrowReleaseStatus
            Assert-Condition ($stageMPPassiveLoopThrowReleaseStatus.Success -and (($pthrs[0] -band 0x1f) -eq 0x1f) -and $pthrs[1] -eq 1 -and $pthrs[2] -eq 1 -and $pthrs[3] -eq 1 -and $pthrs[4] -eq 20 -and $pthrs[5] -eq 26 -and $pthrs[6] -eq 30 -and $pthrs[7] -eq 36 -and $pthrs[8] -eq 40 -and $pthrs[9] -eq 40 -and $pthrs[10] -eq 0 -and $pthrs[11] -eq 1 -and $pthrs[12] -eq 0 -and ([Math]::Abs($pthrs[13]) -eq 1)) 'Stage MP Passive Throw release status proof did not pass.' $gdbStdout
            $pthrp = Get-Ints $stageMPPassiveLoopThrowProcStatus
            Assert-Condition ($stageMPPassiveLoopThrowProcStatus.Success -and (($pthrp[0] -band 0x3f) -eq 0x3f) -and $pthrp[1] -eq 1 -and $pthrp[2] -eq 1 -and $pthrp[3] -eq 1 -and $pthrp[4] -eq 1 -and $pthrp[5] -eq 123) 'Stage MP Passive Throw proc-status callback proof did not pass.' $gdbStdout
            $pthrd = Get-Ints $stageMPPassiveLoopThrowDeadResult
            Assert-Condition ($stageMPPassiveLoopThrowDeadResult.Success -and (($pthrd[0] -band 0x7f) -eq 0x7f) -and $pthrd[1] -eq 1 -and $pthrd[2] -eq 1 -and $pthrd[3] -eq 1 -and $pthrd[4] -eq 2 -and $pthrd[5] -eq 1 -and $pthrd[6] -eq 1 -and $pthrd[7] -eq 10 -and $pthrd[8] -eq 26 -and $pthrd[9] -eq 0 -and $pthrd[10] -eq 1) 'Stage MP Passive Throw dead-result cleanup proof did not pass.' $gdbStdout
            $pwd = Get-Ints $stageMPPassiveLoopWallDamage
            Assert-Condition ($stageMPPassiveLoopWallDamage.Success -and (($pwd[0] -band 0x1ff) -eq 0x1ff) -and $pwd[1] -eq 1 -and $pwd[2] -eq 1 -and $pwd[3] -eq 1 -and $pwd[4] -eq 1 -and $pwd[5] -eq 1 -and $pwd[6] -eq 1 -and $pwd[7] -eq 1 -and $pwd[8] -eq 1 -and $pwd[9] -eq 1 -and $pwd[10] -eq 0) 'Stage MP Passive WallDamage proof did not pass.' $gdbStdout
            $pwds = Get-Ints $stageMPPassiveLoopWallDamageState
            Assert-Condition ($stageMPPassiveLoopWallDamageState.Success -and $pwds[0] -eq 56 -and $pwds[1] -eq 49 -and $pwds[2] -eq 0 -and $pwds[3] -eq 1 -and $pwds[4] -eq 57 -and $pwds[5] -eq 50 -and $pwds[6] -eq 1 -and $pwds[7] -eq 1 -and $pwds[8] -eq 0 -and $pwds[9] -eq 15) 'Stage MP Passive WallDamage state/update markers failed.' $gdbStdout
            $pwdv = Get-Ints $stageMPPassiveLoopWallDamageVec
            Assert-Condition ($stageMPPassiveLoopWallDamageVec.Success -and $pwdv[0] -lt 0 -and $pwdv[1] -gt 0 -and $pwdv[2] -gt 0 -and $pwdv[3] -eq -1 -and $pwdv[4] -eq 5 -and $pwdv[5] -eq 13) 'Stage MP Passive WallDamage vector/source markers failed.' $gdbStdout
            $prb = Get-Ints $stageMPPassiveLoopRebound
            Assert-Condition ($stageMPPassiveLoopRebound.Success -and (($prb[0] -band 0x7f) -eq 0x7f) -and $prb[1] -eq 1 -and $prb[2] -eq 1 -and $prb[3] -eq 1 -and $prb[4] -eq 1 -and $prb[5] -eq 1 -and $prb[6] -eq 1 -and $prb[7] -eq 1) 'Stage MP Passive Rebound proof did not pass.' $gdbStdout
            $prbs = Get-Ints $stageMPPassiveLoopReboundState
            Assert-Condition ($stageMPPassiveLoopReboundState.Success -and $prbs[0] -eq 82 -and $prbs[1] -eq -1 -and $prbs[2] -eq 83 -and $prbs[3] -eq 71 -and $prbs[4] -eq 10 -and $prbs[5] -eq 4) 'Stage MP Passive Rebound state markers failed.' $gdbStdout
            $prbv = Get-Ints $stageMPPassiveLoopReboundVec
            Assert-Condition ($stageMPPassiveLoopReboundVec.Success -and $prbv[0] -eq -12000 -and $prbv[1] -eq 3000 -and $prbv[2] -eq 6000 -and $prbv[3] -eq 0 -and $prbv[4] -eq 1 -and $prbv[5] -eq 1) 'Stage MP Passive Rebound vector/timer markers failed.' $gdbStdout
        }
        $pls = Get-Ints $stageMPPassiveLoopSetup
        Assert-Condition ($stageMPPassiveLoopSetup.Success -and $pls[0] -eq 1 -and $pls[1] -eq 1 -and $pls[2] -eq 1 -and $pls[3] -eq 1 -and $pls[4] -eq 1 -and $pls[5] -eq 1 -and $pls[6] -eq 1 -and $pls[7] -eq 1 -and $pls[8] -eq 1 -and $pls[9] -eq 1 -and $pls[10] -eq 0) 'Stage MP Passive-loop setup markers failed.' $gdbStdout
        $ps = Get-Ints $stageMPPassiveLoopPassiveStand
        Assert-Condition ($stageMPPassiveLoopPassiveStand.Success -and $ps[0] -eq 73 -and $ps[1] -eq 62 -and $ps[2] -eq 0 -and $ps[3] -eq $passiveUpdateTickExpected -and $ps[4] -eq $passiveStableFrameExpected -and $ps[5] -eq $passiveStableFrameExpected -and $ps[6] -eq $passiveStableFrameExpected -and $ps[7] -eq 73 -and $ps[8] -eq 62 -and $ps[9] -eq 0 -and $ps[10] -eq 1 -and $ps[11] -eq 120 -and $ps[12] -eq 1) 'Stage MP PassiveStand stable/final markers failed.' $gdbStdout
        $pn = Get-Ints $stageMPPassiveLoopPassive
        Assert-Condition ($stageMPPassiveLoopPassive.Success -and $pn[0] -eq 81 -and $pn[1] -eq 70 -and $pn[2] -eq 0 -and $pn[3] -eq $passiveUpdateTickExpected -and $pn[4] -eq $passiveStableFrameExpected -and $pn[5] -eq $passiveStableFrameExpected -and $pn[6] -eq $passiveStableFrameExpected -and $pn[7] -eq 81 -and $pn[8] -eq 70 -and $pn[9] -eq 0 -and $pn[10] -eq 1 -and $pn[11] -eq 120 -and $pn[12] -eq 1) 'Stage MP Passive stable/final markers failed.' $gdbStdout
        $pf = Get-Ints $stageMPPassiveLoopFinal
        Assert-Condition ($stageMPPassiveLoopFinal.Success -and $pf[0] -eq 10 -and $pf[1] -eq 4 -and $pf[2] -eq 0 -and $pf[3] -eq 10 -and $pf[4] -eq 4 -and $pf[5] -eq 0 -and $pf[6] -eq 1 -and $pf[7] -eq 1) 'Stage MP Passive-loop final Wait handoff markers failed.' $gdbStdout
        $branchSummary = if ($stageMPPassiveLoopBranch.Success) { " branch=$($stageMPPassiveLoopBranch.Groups[1].Value)" } else { "" }
        $passiveStandBSummary = if ($stageMPPassiveLoopPassiveStandB.Success) { " psb=$($stageMPPassiveLoopPassiveStandB.Groups[1].Value)" } else { "" }
        $naturalMapSummary = if ($stageMPPassiveLoopNaturalMap.Success) { " natural=$($stageMPPassiveLoopNaturalMap.Groups[1].Value)" } else { "" }
        if ($stageMPPassiveLoopNaturalMapCalls.Success) { $naturalMapSummary = "$naturalMapSummary mapcalls=$($stageMPPassiveLoopNaturalMapCalls.Groups[1].Value)" }
        $appealSummary = if ($stageMPPassiveLoopAppeal.Success) { $pa = Get-Ints $stageMPPassiveLoopAppeal; " appeal=$($pa[3])/$($pa[4])" } else { "" }
        $appealGuardSummary = if ($stageMPPassiveLoopAppealGuard.Success) { $pag = Get-Ints $stageMPPassiveLoopAppealGuard; " appealGuard=$($pag[4])/$($pag[5])" } else { "" }
        $catchSummary = if ($stageMPPassiveLoopCatch.Success -and $stageMPPassiveLoopCatchCallbacks.Success) { $pc = Get-Ints $stageMPPassiveLoopCatch; $pcc = Get-Ints $stageMPPassiveLoopCatchCallbacks; " catch=$($pc[4])/$($pc[5])->$($pcc[7])/$($pcc[8])" } else { "" }
        $catchPullSummary = if ($stageMPPassiveLoopCatchPull.Success) { $pcp = Get-Ints $stageMPPassiveLoopCatchPull; " catchPull=$($pcp[10])/$($pcp[11])->$($pcp[13])/-2 tw=$($pcp[16])->$($pcp[17])" } else { "" }
        $captureSummary = if ($stageMPPassiveLoopCapture.Success) { $pcap = Get-Ints $stageMPPassiveLoopCapture; " capture=$($pcap[10])/$($pcap[11])->$($pcap[13])/-2 procDamage=$($pcap[2])" } else { "" }
        $throwSummary = if ($stageMPPassiveLoopThrow.Success) { $pthr = Get-Ints $stageMPPassiveLoopThrow; " throw=$($pthr[6])/$($pthr[7])->$($pthr[10])/$($pthr[11])" } else { "" }
        $throwBSummary = if ($stageMPPassiveLoopThrowB.Success) { $pthrb = Get-Ints $stageMPPassiveLoopThrowB; " throwB=$($pthrb[1])/$($pthrb[2])->$($pthrb[5])/$($pthrb[6])" } else { "" }
        $throwCallbackSummary = if ($stageMPPassiveLoopThrowCallback.Success) { $pthrc = Get-Ints $stageMPPassiveLoopThrowCallback; " throwCb=$($pthrc[1])/$($pthrc[2])/$($pthrc[3]) floor=$($pthrc[7]) end=$($pthrc[9])/$($pthrc[13])/$($pthrc[14])" } else { "" }
        $throwUpdateSummary = if ($stageMPPassiveLoopThrowUpdate.Success) { $pthru = Get-Ints $stageMPPassiveLoopThrowUpdate; " throwUpdate=$($pthru[3])/$($pthru[4]) dmg=$($pthru[9])->$($pthru[10]) script=$($pthru[14])" } else { "" }
        $throwReleaseSummary = if ($stageMPPassiveLoopThrowRelease.Success) { $pthrr = Get-Ints $stageMPPassiveLoopThrowRelease; " throwRelease=dmg=$($pthrr[12])->$($pthrr[13]) kb=$($pthrr[15]) script=$($pthrr[17])" } else { "" }
        $throwReleaseStatusSummary = if ($stageMPPassiveLoopThrowReleaseStatus.Success) { $pthrs = Get-Ints $stageMPPassiveLoopThrowReleaseStatus; " throwReleaseStatus=dmg=$($pthrs[4])->$($pthrs[5]) upd=$($pthrs[6])->$($pthrs[7]) noDmg=$($pthrs[8])->$($pthrs[9])" } else { "" }
        $throwProcStatusSummary = if ($stageMPPassiveLoopThrowProcStatus.Success) { $pthrp = Get-Ints $stageMPPassiveLoopThrowProcStatus; " throwProc=param=$($pthrp[2]) script=$($pthrp[5])" } else { "" }
        $throwDeadSummary = if ($stageMPPassiveLoopThrowDeadResult.Success) { $pthrd = Get-Ints $stageMPPassiveLoopThrowDeadResult; " throwDead=call=$($pthrd[1]) coll=$($pthrd[2]) air=$($pthrd[3]) waitFall=$($pthrd[4]) status=$($pthrd[7])/$($pthrd[8])" } else { "" }
        $wallDamageSummary = if ($stageMPPassiveLoopWallDamageState.Success) { $pwds = Get-Ints $stageMPPassiveLoopWallDamageState; " wallDamage=$($pwds[0])/$($pwds[1])->$($pwds[4])/$($pwds[5])" } else { "" }
        $reboundSummary = if ($stageMPPassiveLoopReboundState.Success) { $prbs = Get-Ints $stageMPPassiveLoopReboundState; " rebound=$($prbs[0])/$($prbs[1])->$($prbs[2])/$($prbs[3])->$($prbs[4])/$($prbs[5])" } else { "" }
        $stageSummary = "$stageSummary mpPassive=ps=$($ps[0])/$($ps[1])->$($pf[0])/$($pf[1]) ticks=$($ps[3])/$($ps[4])/$($ps[5]) passive=$($pn[0])/$($pn[1])->$($pf[3])/$($pf[4]) ticks=$($pn[3])/$($pn[4])/$($pn[5])$branchSummary$passiveStandBSummary$naturalMapSummary$appealSummary$appealGuardSummary$catchSummary$catchPullSummary$captureSummary$throwSummary$throwBSummary$throwCallbackSummary$throwUpdateSummary$throwReleaseSummary$throwReleaseStatusSummary$throwProcStatusSummary$throwDeadSummary$wallDamageSummary$reboundSummary"
    }
    if ($RequireStageMPDownWaitLoop) {
        $dw = Get-Ints $stageMPDownWaitLoop
        Assert-Condition ($stageMPDownWaitLoop.Success -and $dw[0] -eq 0x46445749 -and $dw[1] -eq 0x46445755 -and (($dw[2] -band 0x3ffff) -eq 0x3ffff) -and $dw[3] -eq 0x3ffff -and $dw[4] -eq 1) 'Stage MP DownWait interrupt-loop result/mask did not pass.' $gdbStdout
        $dws = Get-Ints $stageMPDownWaitLoopSetup
        Assert-Condition ($stageMPDownWaitLoopSetup.Success -and $dws[0] -eq 1 -and $dws[1] -eq 1 -and $dws[2] -eq 1 -and $dws[3] -eq 1 -and ($dws[4] -eq 1 -or $dws[4] -eq 2) -and $dws[5] -eq 70 -and $dws[6] -eq -2 -and $dws[7] -eq 0 -and $dws[8] -eq 180 -and $dws[9] -eq 0x33 -and $dws[10] -eq 500 -and $dws[11] -eq 1) 'Stage MP DownWait setup markers failed.' $gdbStdout
        $dwi = Get-Ints $stageMPDownWaitLoopInterrupt
        Assert-Condition ($stageMPDownWaitLoopInterrupt.Success -and $dwi[0] -eq 1 -and $dwi[1] -eq 1 -and $dwi[2] -eq 1 -and $dwi[3] -eq 1 -and $dwi[4] -eq 1 -and $dwi[5] -eq 1 -and $dwi[6] -eq 0x12345 -and $dwi[7] -eq 0) 'Stage MP DownWait source-order interrupt markers failed.' $gdbStdout
        $dwin = Get-Ints $stageMPDownWaitLoopInput
        Assert-Condition ($stageMPDownWaitLoopInput.Success -and $dwin[0] -eq 0 -and $dwin[1] -ge 20 -and $dwin[2] -eq 0 -and $dwin[3] -ne 0) 'Stage MP DownWait input markers failed.' $gdbStdout
        $dwst = Get-Ints $stageMPDownWaitLoopStatus
        Assert-Condition ($stageMPDownWaitLoopStatus.Success -and $dwst[0] -eq 70 -and $dwst[1] -eq -2 -and $dwst[2] -eq 0 -and $dwst[3] -eq 180 -and $dwst[4] -eq 1 -and $dwst[5] -eq 72 -and $dwst[6] -eq 61 -and $dwst[7] -eq 0 -and $dwst[9] -eq 0 -and $dwst[10] -eq 1000 -and $dwst[11] -eq 1) 'Stage MP DownWait final DownStand markers failed.' $gdbStdout
        $dwsdt = Get-Ints $stageMPDownWaitLoopDownStandTick
        Assert-Condition ($stageMPDownWaitLoopDownStandTick.Success -and $dwsdt[0] -eq 8 -and $dwsdt[1] -eq 8 -and $dwsdt[2] -eq 8 -and $dwsdt[3] -eq 8 -and $dwsdt[4] -eq 1 -and $dwsdt[5] -eq 9 -and $dwsdt[6] -eq 8 -and $dwsdt[7] -eq 8 -and $dwsdt[8] -eq 8 -and $dwsdt[9] -eq 72 -and $dwsdt[10] -eq 61 -and $dwsdt[11] -eq 0 -and $dwsdt[12] -eq 1 -and $dwsdt[13] -eq 1 -and $dwsdt[14] -eq 10 -and $dwsdt[15] -eq 4 -and $dwsdt[16] -eq 0 -and $dwsdt[17] -eq 120 -and $dwsdt[18] -eq 1) 'Stage MP DownWait DownStand callback/final Wait markers failed.' $gdbStdout
        $dwa = Get-Ints $stageMPDownWaitLoopAttack
        Assert-Condition ($stageMPDownWaitLoopAttack.Success -and $dwa[0] -eq 1 -and $dwa[1] -eq 1 -and $dwa[2] -eq 1 -and $dwa[3] -eq 1 -and $dwa[4] -eq 1 -and $dwa[5] -eq 0x1234 -and $dwa[6] -ne 0 -and $dwa[7] -ne 0 -and $dwa[8] -eq 70 -and $dwa[9] -eq -2 -and $dwa[10] -eq 0 -and $dwa[11] -eq 80 -and $dwa[12] -eq 69 -and $dwa[13] -eq 0 -and $dwa[14] -eq 53 -and $dwa[15] -eq 33 -and $dwa[16] -eq 33 -and $dwa[17] -eq 1) 'Stage MP DownWait attack branch markers failed.' $gdbStdout
        $dwat = Get-Ints $stageMPDownWaitLoopAttackTick
        Assert-Condition ($stageMPDownWaitLoopAttackTick.Success -and $dwat[0] -eq 9 -and $dwat[1] -eq 8 -and $dwat[2] -eq 8 -and $dwat[3] -eq 8 -and $dwat[4] -eq 80 -and $dwat[5] -eq 69 -and $dwat[6] -eq 0 -and $dwat[7] -eq 1 -and $dwat[8] -eq 1 -and $dwat[9] -eq 10 -and $dwat[10] -eq 4 -and $dwat[11] -eq 0 -and $dwat[12] -eq 120 -and $dwat[13] -eq 1) 'Stage MP DownWait attack callback/final Wait markers failed.' $gdbStdout
        $dwr = Get-Ints $stageMPDownWaitLoopRoll
        Assert-Condition ($stageMPDownWaitLoopRoll.Success -and $dwr[0] -eq 2 -and $dwr[1] -eq 2 -and $dwr[2] -eq 2 -and $dwr[3] -eq 2 -and $dwr[4] -eq 2 -and $dwr[5] -eq 2 -and $dwr[6] -eq 0x12345 -and $dwr[7] -eq 0x12345) 'Stage MP DownWait roll source-order markers failed.' $gdbStdout
        $dwrs = Get-Ints $stageMPDownWaitLoopRollStatus
        Assert-Condition ($stageMPDownWaitLoopRollStatus.Success -and $dwrs[0] -ge 20 -and $dwrs[1] -eq 0 -and $dwrs[2] -eq 76 -and $dwrs[3] -eq 65 -and $dwrs[4] -eq 0 -and $dwrs[5] -eq 1 -and $dwrs[6] -eq 1 -and $dwrs[7] -le -20 -and $dwrs[8] -eq 0 -and $dwrs[9] -eq 78 -and $dwrs[10] -eq 67 -and $dwrs[11] -eq 0 -and $dwrs[12] -eq 1 -and $dwrs[13] -eq 1) 'Stage MP DownWait roll status markers failed.' $gdbStdout
        $dwrf = Get-Ints $stageMPDownWaitLoopRollForwardTick
        Assert-Condition ($stageMPDownWaitLoopRollForwardTick.Success -and $dwrf[0] -eq 9 -and $dwrf[1] -eq 8 -and $dwrf[2] -eq 8 -and $dwrf[3] -eq 8 -and $dwrf[4] -eq 76 -and $dwrf[5] -eq 65 -and $dwrf[6] -eq 0 -and $dwrf[7] -eq 1 -and $dwrf[8] -eq 1 -and $dwrf[9] -eq 10 -and $dwrf[10] -eq 4 -and $dwrf[11] -eq 0 -and $dwrf[12] -eq 120 -and $dwrf[13] -eq 1) 'Stage MP DownWait forward-roll callback/final Wait markers failed.' $gdbStdout
        $dwrb = Get-Ints $stageMPDownWaitLoopRollBackTick
        Assert-Condition ($stageMPDownWaitLoopRollBackTick.Success -and $dwrb[0] -eq 9 -and $dwrb[1] -eq 8 -and $dwrb[2] -eq 8 -and $dwrb[3] -eq 8 -and $dwrb[4] -eq 78 -and $dwrb[5] -eq 67 -and $dwrb[6] -eq 0 -and $dwrb[7] -eq 1 -and $dwrb[8] -eq 1 -and $dwrb[9] -eq 10 -and $dwrb[10] -eq 4 -and $dwrb[11] -eq 0 -and $dwrb[12] -eq 120 -and $dwrb[13] -eq 1) 'Stage MP DownWait back-roll callback/final Wait markers failed.' $gdbStdout
        $dwrm = Get-Ints $stageMPDownWaitLoopRollMove
        Assert-Condition ($stageMPDownWaitLoopRollMove.Success -and $dwrm[2] -gt 0 -and $dwrm[5] -lt 0 -and $dwrm[6] -eq 0x3) 'Stage MP DownWait roll root movement markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpDownWait=status=$($dwst[0])/$($dwst[1])->$($dwst[5])/$($dwst[6])->$($dwsdt[14])/$($dwsdt[15]) attack=$($dwa[11])/$($dwa[12])->$($dwat[9])/$($dwat[10]) rolls=$($dwrs[2])/$($dwrs[3])->$($dwrf[9])/$($dwrf[10]),$($dwrs[9])/$($dwrs[10])->$($dwrb[9])/$($dwrb[10]) ticks=$($dwat[0])/$($dwrf[0])/$($dwrb[0]) rollDelta=$($dwrm[2])/$($dwrm[5]) downStandTicks=$($dwsdt[5])/$($dwsdt[6])/$($dwsdt[7]) source=0x$('{0:x}' -f $dwi[6]) checks=$($dwi[1])/$($dwi[2])/$($dwi[3]) dsChecks=$($dwsdt[1])/$($dwsdt[2])/$($dwsdt[3]) input=$($dwin[0]),$($dwin[1]) flag1=$($dwst[4])->$($dwst[9])/$($dwsdt[4])"
    }
    if ($RequireStageTurnLoop) {
        $tu = Get-Ints $stageTurnLoop
        Assert-Condition ($stageTurnLoop.Success -and $tu[0] -eq 0x46545552 -and $tu[1] -eq 0x46545553 -and (($tu[2] -band 0x1ff) -eq 0x1ff) -and $tu[3] -eq 0xff -and $tu[4] -eq 1) 'Stage Turn-loop result/mask did not pass.' $gdbStdout
        $tc = Get-Ints $stageTurnLoopCalls
        Assert-Condition ($stageTurnLoopCalls.Success -and $tc[0] -eq 1 -and $tc[1] -eq 1 -and $tc[2] -eq 1 -and $tc[3] -eq 1 -and $tc[4] -eq 1 -and $tc[5] -eq 1 -and $tc[6] -eq 1 -and $tc[7] -eq 1 -and $tc[8] -eq 1 -and $tc[9] -eq 1 -and $tc[10] -eq 1 -and $tc[11] -eq 0) 'Stage Turn-loop source-order call counts failed.' $gdbStdout
        $tin = Get-Ints $stageTurnLoopInput
        Assert-Condition ($stageTurnLoopInput.Success -and $tin[0] -le -20 -and $tin[1] -eq 10 -and $tin[2] -eq 4 -and $tin[3] -eq 0 -and $tin[4] -eq 1 -and $tin[5] -gt 0 -and $tin[6] -eq 1) 'Stage Turn-loop input/before markers failed.' $gdbStdout
        $tset = Get-Ints $stageTurnLoopSetup
        Assert-Condition ($stageTurnLoopSetup.Success -and $tset[0] -eq 18 -and $tset[1] -eq 12 -and $tset[2] -eq 0 -and $tset[3] -eq 1 -and $tset[4] -gt 0 -and $tset[5] -eq 0 -and $tset[6] -eq 0 -and $tset[7] -eq 0 -and $tset[8] -eq 0 -and $tset[9] -eq -1 -and $tset[10] -eq 256) 'Stage Turn-loop setup markers failed.' $gdbStdout
        $tup = Get-Ints $stageTurnLoopUpdate
        Assert-Condition ($stageTurnLoopUpdate.Success -and $tup[0] -eq 18 -and $tup[1] -eq 12 -and $tup[2] -eq 0 -and $tup[3] -eq -1 -and $tup[4] -lt 0 -and $tup[5] -eq 0 -and $tup[6] -eq 1 -and $tup[7] -eq 1) 'Stage Turn-loop update flip markers failed.' $gdbStdout
        $tf = Get-Ints $stageTurnLoopFinal
        Assert-Condition ($stageTurnLoopFinal.Success -and $tf[0] -eq 10 -and $tf[1] -eq 4 -and $tf[2] -eq 0 -and $tf[3] -eq -1 -and $tf[4] -le 0 -and $tf[5] -eq 1 -and $tf[6] -eq 1 -and $tf[7] -eq 120 -and $tf[8] -eq 1) 'Stage Turn-loop final Wait handoff markers failed.' $gdbStdout
        $stageSummary = "$stageSummary turn=$($tin[1])/$($tin[2])->$($tset[0])/$($tset[1])->$($tf[0])/$($tf[1]) lr=$($tin[4])->$($tup[3]) vel=$($tin[5])->$($tup[4]) calls=$($tc[2])/$($tc[7])/$($tc[8])"
    }
    if ($RequireStageMPDownRecoverLoop) {
        $drp = Get-Ints $stageMPDownRecoverLoop
        Assert-Condition ($stageMPDownRecoverLoop.Success -and $drp[0] -eq 0x46445250 -and $drp[1] -eq 0x46445253 -and (($drp[2] -band 0xff) -eq 0xff) -and $drp[3] -eq 0xff -and $drp[4] -eq 1) 'Stage MP down-recover result/mask did not pass.' $gdbStdout
        $drs = Get-Ints $stageMPDownRecoverLoopSetup
        Assert-Condition ($stageMPDownRecoverLoopSetup.Success -and $drs[0] -eq 1 -and $drs[1] -eq 1 -and $drs[2] -eq 1 -and $drs[3] -eq 1 -and ($drs[4] -eq 1 -or $drs[4] -eq 2) -and $drs[5] -eq 69 -and $drs[6] -eq -2 -and $drs[7] -eq 0 -and $drs[8] -eq 180 -and $drs[9] -eq 1 -and $drs[10] -eq 0) 'Stage MP down-recover DownWaitD setup markers failed.' $gdbStdout
        $drd = Get-Ints $stageMPDownRecoverLoopDownStand
        Assert-Condition ($stageMPDownRecoverLoopDownStand.Success -and $drd[0] -eq 1 -and $drd[1] -eq 1 -and $drd[2] -eq 1 -and $drd[3] -eq 1 -and $drd[4] -eq 1 -and $drd[5] -eq 1 -and $drd[6] -eq 0x12345 -and $drd[7] -eq 71 -and $drd[8] -eq 60 -and $drd[9] -eq 0 -and $drd[10] -eq 1) 'Stage MP down-recover DownStandD source-order/status markers failed.' $gdbStdout
        $dra = Get-Ints $stageMPDownRecoverLoopAttack
        Assert-Condition ($stageMPDownRecoverLoopAttack.Success -and $dra[0] -eq 1 -and $dra[1] -eq 1 -and $dra[2] -eq 1 -and $dra[3] -eq 1 -and $dra[4] -eq 1 -and $dra[5] -eq 0x1234 -and $dra[6] -eq 79 -and $dra[7] -eq 68 -and $dra[8] -eq 0 -and $dra[9] -eq 52 -and $dra[10] -eq 32 -and $dra[11] -eq 32 -and $dra[12] -eq 1) 'Stage MP down-recover DownAttackD source-order/status markers failed.' $gdbStdout
        $drr = Get-Ints $stageMPDownRecoverLoopRoll
        Assert-Condition ($stageMPDownRecoverLoopRoll.Success -and $drr[0] -eq 2 -and $drr[1] -eq 2 -and $drr[2] -eq 2 -and $drr[3] -eq 2 -and $drr[4] -eq 2 -and $drr[5] -eq 2 -and $drr[6] -eq 0x12345 -and $drr[7] -eq 0x12345 -and $drr[8] -eq 75 -and $drr[9] -eq 64 -and $drr[10] -eq 0 -and $drr[11] -eq 1 -and $drr[12] -eq 77 -and $drr[13] -eq 66 -and $drr[14] -eq 0 -and $drr[15] -eq 1) 'Stage MP down-recover DownForwardD/DownBackD source-order/status markers failed.' $gdbStdout
        $drf = Get-Ints $stageMPDownRecoverLoopFinal
        Assert-Condition ($stageMPDownRecoverLoopFinal.Success -and $drf[0] -eq 4 -and $drf[1] -eq 4 -and (($drf[2] -band 0xf) -eq 0xf) -and (($drf[3] -band 0xf) -eq 0xf) -and (($drf[4] -band 0xf) -eq 0xf) -and $drf[5] -eq 0) 'Stage MP down-recover final Wait handoff markers failed.' $gdbStdout
        $stageSummary = "$stageSummary downRecoverD=wait=$($drs[5])/$($drs[6]) stand=$($drd[7])/$($drd[8]) atk=$($dra[6])/$($dra[7]) roll=$($drr[8])/$($drr[9]),$($drr[12])/$($drr[13]) final=0x$('{0:x}' -f $drf[2])"
    }
    if ($RequireStageMPCliffLedgeLoop) {
        $led = Get-Ints $stageMPCliffLedgeLoop
        Assert-Condition ($stageMPCliffLedgeLoop.Success -and $led[0] -eq 0x464c4750 -and $led[1] -eq 0x464c4753 -and (($led[2] -band 0x3ff) -eq 0x3ff) -and $led[3] -eq 0xff -and $led[4] -eq 1) 'Stage MP cliff-ledge result/mask did not pass.' $gdbStdout
        $leb = Get-Ints $stageMPCliffLedgeLoopBase
        Assert-Condition ($stageMPCliffLedgeLoopBase.Success -and $leb[0] -eq 1 -and $leb[1] -eq 1 -and $leb[2] -eq 1 -and $leb[3] -eq 1 -and $leb[4] -eq 1 -and $leb[5] -eq 1 -and $leb[6] -ge 0 -and $leb[7] -eq $leb[6]) 'Stage MP cliff-ledge base/occupancy markers failed.' $gdbStdout
        $les = Get-Ints $stageMPCliffLedgeLoopState
        Assert-Condition ($stageMPCliffLedgeLoopState.Success -and $les[0] -eq 26 -and $les[1] -eq 20 -and $les[2] -eq 1 -and $les[3] -ge 0 -and $les[4] -eq 30 -and $les[5] -eq 0 -and $les[6] -eq 1 -and $les[7] -eq 84 -and $les[8] -eq 72 -and $les[9] -eq 1 -and $les[10] -eq $les[3] -and $les[11] -eq 1 -and $les[12] -eq 0 -and $les[13] -eq 10 -and $les[14] -eq 4 -and $les[15] -eq 0 -and $les[16] -eq 0) 'Stage MP cliff-ledge drop/recatch/climb markers failed.' $gdbStdout
        $lel = Get-Ints $stageMPCliffLedgeLoopLines
        Assert-Condition ($stageMPCliffLedgeLoopLines.Success -and $lel[0] -ge 0 -and $lel[1] -eq $lel[0] -and $lel[2] -eq $lel[0]) 'Stage MP cliff-ledge line continuity markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffLedge=occ=$($leb[5]) drop=$($les[0])/$($les[1]) wait=$($les[4]) hold=$($les[5]) recatch=$($les[7])/$($les[8]) finish=$($les[13])/$($les[14]) line=$($lel[0])"
    }
    if ($RequireStageMPCliffLiveLoop) {
        $liv = Get-Ints $stageMPCliffLiveLoop
        Assert-Condition ($stageMPCliffLiveLoop.Success -and $liv[0] -eq 0x464c5650 -and $liv[1] -eq 0x464c5653 -and (($liv[2] -band 0x3ff) -eq 0x3ff) -and $liv[3] -eq 0xff -and $liv[4] -eq 1) 'Stage MP cliff-live result/mask did not pass.' $gdbStdout
        $lvb = Get-Ints $stageMPCliffLiveLoopBase
        Assert-Condition ($stageMPCliffLiveLoopBase.Success -and $lvb[0] -eq 1 -and $lvb[1] -eq 1 -and $lvb[2] -eq 1 -and $lvb[3] -ge 7 -and $lvb[4] -ge 7 -and (($lvb[5] -band 0xfff) -eq 0xfff)) 'Stage MP cliff-live base/process/source markers failed.' $gdbStdout
        $lvc = Get-Ints $stageMPCliffLiveLoopCounts
        Assert-Condition ($stageMPCliffLiveLoopCounts.Success -and $lvc[0] -eq 1 -and $lvc[1] -eq 1 -and $lvc[2] -eq 1 -and $lvc[3] -eq 1 -and $lvc[4] -eq 1 -and $lvc[5] -eq 1 -and $lvc[6] -eq 1 -and $lvc[7] -eq 1 -and $lvc[8] -eq 1) 'Stage MP cliff-live callback counts failed.' $gdbStdout
        $lvs = Get-Ints $stageMPCliffLiveLoopStatus
        Assert-Condition ($stageMPCliffLiveLoopStatus.Success -and $lvs[0] -eq 84 -and $lvs[1] -eq 72 -and $lvs[2] -eq 1 -and $lvs[3] -eq 85 -and $lvs[4] -eq 73 -and $lvs[5] -eq 0 -and $lvs[6] -eq 0 -and $lvs[7] -gt 1 -and $lvs[8] -ge 0 -and $lvs[9] -eq 86 -and $lvs[10] -eq 74 -and $lvs[11] -eq 0 -and $lvs[12] -eq 0 -and $lvs[13] -eq $lvs[8] -and $lvs[14] -eq 87 -and $lvs[15] -eq 75 -and $lvs[16] -eq 88 -and $lvs[17] -eq 76 -and $lvs[18] -eq $lvs[8]) 'Stage MP cliff-live status chain markers failed.' $gdbStdout
        $lvf = Get-Ints $stageMPCliffLiveLoopFinal
        Assert-Condition ($stageMPCliffLiveLoopFinal.Success -and $lvf[0] -eq 0 -and $lvf[1] -eq 88 -and $lvf[2] -eq 76 -and $lvf[3] -eq 0 -and $lvf[4] -eq 10 -and $lvf[5] -eq 4 -and $lvf[6] -eq 0 -and $lvf[7] -eq 26 -and $lvf[8] -eq 20 -and $lvf[9] -eq 1) 'Stage MP cliff-live common2/finish/drop status markers failed.' $gdbStdout
        $lvd = Get-Ints $stageMPCliffLiveLoopDrop
        Assert-Condition ($stageMPCliffLiveLoopDrop.Success -and $lvd[0] -eq 30 -and $lvd[1] -eq 0 -and $lvd[2] -eq 0) 'Stage MP cliff-live drop safety markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffLive=wait=$($lvs[3])/$($lvs[4]) climb=$($lvs[9])/$($lvs[10]) quick2=$($lvs[16])/$($lvs[17]) finish=$($lvf[4])/$($lvf[5]) drop=$($lvf[7])/$($lvf[8]) src=0x$('{0:x}' -f $lvb[5])"
    }
    if ($RequireStageMPCliffAttackAction) {
        $caa = Get-Ints $stageMPCliffAttackAction
        Assert-Condition ($stageMPCliffAttackAction.Success -and $caa[0] -eq 0x46434155 -and $caa[1] -eq 0x46434156 -and (($caa[2] -band 0xfff) -eq 0xfff) -and $caa[3] -eq 0xff -and $caa[4] -eq 1) 'Stage MP cliff-attack action result/mask did not pass.' $gdbStdout
        $caac = Get-Ints $stageMPCliffAttackActionCalls
        Assert-Condition ($stageMPCliffAttackActionCalls.Success -and $caac[0] -eq 1 -and $caac[1] -eq 1 -and $caac[2] -eq 1 -and $caac[3] -eq 1 -and $caac[4] -eq 1 -and $caac[5] -eq 1 -and $caac[6] -eq 1 -and $caac[7] -eq 1 -and $caac[8] -eq 1 -and $caac[9] -eq 0) 'Stage MP cliff-attack action source-order call counts failed.' $gdbStdout
        $caas = Get-Ints $stageMPCliffAttackActionStatus
        Assert-Condition ($stageMPCliffAttackActionStatus.Success -and $caas[0] -eq 86 -and $caas[1] -eq 74 -and $caas[2] -eq 0 -and $caas[3] -eq 92 -and $caas[4] -eq 80 -and $caas[5] -eq 0 -and $caas[6] -eq 93 -and $caas[7] -eq 81 -and $caas[8] -eq 0 -and $caas[9] -ge 0 -and $caas[10] -eq $caas[9] -and $caas[11] -eq $caas[9] -and $caas[12] -eq $caas[9] -and $caas[13] -eq 1 -and $caas[14] -eq $caas[9]) 'Stage MP cliff-attack action status/ledge/floor-copyback markers failed.' $gdbStdout
        $caaf = Get-Ints $stageMPCliffAttackActionFlags
        Assert-Condition ($stageMPCliffAttackActionFlags.Success -and $caaf[0] -eq 1 -and $caaf[1] -eq 0 -and $caaf[2] -eq 1 -and $caaf[3] -eq 0 -and $caaf[4] -eq 1 -and $caaf[5] -eq 1 -and $caaf[6] -eq 1) 'Stage MP cliff-attack action callback/flag markers failed.' $gdbStdout
        if (-not $RequireStageMPCliffEscapeAction) {
            Assert-CliffCommon2BridgeRoot $stageMPCliffAttackActionRoot 1 'Stage MP cliff-attack action common2 bridge/root markers failed.' $gdbStdout
        }
        $caar = Get-Ints $stageMPCliffAttackActionRoot
        $stageSummary = "$stageSummary mpCliffAttackAction=status=$($caas[0])->$($caas[3])->$($caas[6]) motion=$($caas[1])->$($caas[4])->$($caas[7]) cliff=$($caas[9]) floor=$($caas[12]) calls=$($caac[2])/$($caac[4])/$($caac[6])"
        $stageSummary = "$stageSummary attackBridge=root=$($caar[6]),$($caar[7])->$($caar[8]),$($caar[9]) exp=$($caar[10]),$($caar[11])"
    }
    if ($RequireStageMPCliffCommon2) {
        $cc2 = Get-Ints $stageMPCliffCommon2
        Assert-Condition ($stageMPCliffCommon2.Success -and $cc2[0] -eq 0x46433250 -and $cc2[1] -eq 0x46433253 -and (($cc2[2] -band 0x1ff) -eq 0x1ff) -and $cc2[3] -eq 0xff -and $cc2[4] -eq 1) 'Stage MP cliff-common2 result/mask did not pass.' $gdbStdout
        $cc2c = Get-Ints $stageMPCliffCommon2Calls
        Assert-Condition ($stageMPCliffCommon2Calls.Success -and $cc2c[0] -eq 1 -and $cc2c[1] -eq 1 -and $cc2c[2] -eq 1 -and $cc2c[3] -eq 1 -and $cc2c[4] -eq 0 -and $cc2c[5] -eq 1 -and $cc2c[6] -eq 1 -and $cc2c[7] -eq 1 -and $cc2c[8] -eq 1 -and $cc2c[9] -eq 0) 'Stage MP cliff-common2 callback/seam counts failed.' $gdbStdout
        $cc2s = Get-Ints $stageMPCliffCommon2Status
        Assert-Condition ($stageMPCliffCommon2Status.Success -and $cc2s[0] -eq 93 -and $cc2s[1] -eq 81 -and $cc2s[2] -eq 0 -and $cc2s[3] -eq 93 -and $cc2s[4] -eq 81 -and $cc2s[5] -eq 0 -and $cc2s[6] -eq 93 -and $cc2s[7] -eq 81 -and $cc2s[8] -eq 0 -and $cc2s[9] -eq 93 -and $cc2s[10] -eq 81 -and $cc2s[11] -eq 0) 'Stage MP cliff-common2 before/after status markers failed.' $gdbStdout
        $cc2f = Get-Ints $stageMPCliffCommon2Flags
        Assert-Condition ($stageMPCliffCommon2Flags.Success -and $cc2f[0] -ge 0 -and $cc2f[1] -eq $cc2f[0] -and $cc2f[2] -eq $cc2f[0] -and $cc2f[3] -eq $cc2f[1] -and $cc2f[4] -eq 1 -and $cc2f[5] -eq 1 -and $cc2f[6] -eq 1 -and $cc2f[7] -eq 0) 'Stage MP cliff-common2 floor/proc markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffCommon2=status=$($cc2s[0])->$($cc2s[3])->$($cc2s[6])->$($cc2s[9]) motion=$($cc2s[1])->$($cc2s[4])->$($cc2s[7])->$($cc2s[10]) cliff=$($cc2f[0]) floor=$($cc2f[1])->$($cc2f[3]) calls=$($cc2c[2])/$($cc2c[5])/$($cc2c[7])"
    }
    if ($RequireStageMPCliffEscapeAction) {
        $cea = Get-Ints $stageMPCliffEscapeAction
        Assert-Condition ($stageMPCliffEscapeAction.Success -and $cea[0] -eq 0x46434550 -and $cea[1] -eq 0x46434553 -and (($cea[2] -band 0x3fff) -eq 0x3fff) -and $cea[3] -eq 0xff -and $cea[4] -eq 1) 'Stage MP cliff-escape action result/mask did not pass.' $gdbStdout
        $ceac = Get-Ints $stageMPCliffEscapeActionCalls
        Assert-Condition ($stageMPCliffEscapeActionCalls.Success -and $ceac[0] -eq 1 -and $ceac[1] -eq 1 -and $ceac[2] -eq 1 -and $ceac[3] -eq 1 -and $ceac[4] -eq 1 -and $ceac[5] -eq 0 -and $ceac[6] -eq 1 -and $ceac[7] -eq 1 -and $ceac[8] -eq 1 -and $ceac[9] -eq 1 -and $ceac[10] -eq 1 -and $ceac[11] -eq 1 -and $ceac[12] -eq 1 -and $ceac[13] -eq 1 -and $ceac[14] -eq 1 -and $ceac[15] -eq 0) 'Stage MP cliff-escape action source-order call counts failed.' $gdbStdout
        $ceas = Get-Ints $stageMPCliffEscapeActionStatus
        Assert-Condition ($stageMPCliffEscapeActionStatus.Success -and $ceas[0] -eq 85 -and $ceas[1] -eq 73 -and $ceas[2] -eq 0 -and $ceas[3] -eq 86 -and $ceas[4] -eq 74 -and $ceas[5] -eq 0 -and $ceas[6] -eq 96 -and $ceas[7] -eq 84 -and $ceas[8] -eq 0 -and $ceas[9] -eq 97 -and $ceas[10] -eq 85 -and $ceas[11] -eq 0) 'Stage MP cliff-escape action status markers failed.' $gdbStdout
        $ceal = Get-Ints $stageMPCliffEscapeActionLedge
        Assert-Condition ($stageMPCliffEscapeActionLedge.Success -and $ceal[0] -ge 0 -and $ceal[1] -eq $ceal[0] -and $ceal[2] -eq $ceal[0] -and $ceal[3] -eq $ceal[0] -and $ceal[4] -eq $ceal[0] -and $ceal[5] -eq 2 -and $ceal[6] -eq $ceal[0]) 'Stage MP cliff-escape action ledge/queue markers failed.' $gdbStdout
        $ceaf = Get-Ints $stageMPCliffEscapeActionFlags
        Assert-Condition ($stageMPCliffEscapeActionFlags.Success -and $ceaf[0] -eq 1 -and $ceaf[1] -eq 1 -and $ceaf[2] -eq 0 -and $ceaf[5] -eq 1 -and $ceaf[6] -eq 1 -and $ceaf[7] -eq 0 -and $ceaf[8] -eq 1 -and $ceaf[9] -eq 1 -and $ceaf[10] -eq 1 -and $ceaf[11] -eq 1 -and $ceaf[12] -gt 0 -and $ceaf[14] -eq 0 -and $ceaf[15] -ne 0 -and $ceaf[15] -eq $ceaf[16] -and $ceaf[15] -ne $ceaf[17]) 'Stage MP cliff-escape action flag/input markers failed.' $gdbStdout
        Assert-CliffCommon2BridgeRoot $stageMPCliffEscapeActionRoot 2 'Stage MP cliff-escape action common2 bridge/root markers failed.' $gdbStdout
        $cear = Get-Ints $stageMPCliffEscapeActionRoot
        $stageSummary = "$stageSummary mpCliffEscapeAction=status=$($ceas[0])->$($ceas[3])->$($ceas[6])->$($ceas[9]) motion=$($ceas[1])->$($ceas[4])->$($ceas[7])->$($ceas[10]) cliff=$($ceal[0]) floor=$($ceal[4]) calls=$($ceac[2])/$($ceac[4])/$($ceac[10])"
        $stageSummary = "$stageSummary escapeBridge=root=$($cear[6]),$($cear[7])->$($cear[8]),$($cear[9]) exp=$($cear[10]),$($cear[11])"
    }
    if ($RequireStageMPCliffEscapeCommon2) {
        $ce2 = Get-Ints $stageMPCliffEscapeCommon2
        Assert-Condition ($stageMPCliffEscapeCommon2.Success -and $ce2[0] -eq 0x46453250 -and $ce2[1] -eq 0x46453253 -and (($ce2[2] -band 0x1ff) -eq 0x1ff) -and $ce2[3] -eq 0xff -and $ce2[4] -eq 1) 'Stage MP cliff-escape common2 result/mask did not pass.' $gdbStdout
        $ce2c = Get-Ints $stageMPCliffEscapeCommon2Calls
        Assert-Condition ($stageMPCliffEscapeCommon2Calls.Success -and $ce2c[0] -eq 1 -and $ce2c[1] -eq 1 -and $ce2c[2] -eq 1 -and $ce2c[3] -eq 1 -and $ce2c[4] -eq 0 -and $ce2c[5] -eq 1 -and $ce2c[6] -eq 1 -and $ce2c[7] -eq 1 -and $ce2c[8] -eq 1 -and $ce2c[9] -eq 0) 'Stage MP cliff-escape common2 callback/seam counts failed.' $gdbStdout
        $ce2s = Get-Ints $stageMPCliffEscapeCommon2Status
        Assert-Condition ($stageMPCliffEscapeCommon2Status.Success -and $ce2s[0] -eq 97 -and $ce2s[1] -eq 85 -and $ce2s[2] -eq 0 -and $ce2s[3] -eq 97 -and $ce2s[4] -eq 85 -and $ce2s[5] -eq 0 -and $ce2s[6] -eq 97 -and $ce2s[7] -eq 85 -and $ce2s[8] -eq 0 -and $ce2s[9] -eq 97 -and $ce2s[10] -eq 85 -and $ce2s[11] -eq 0) 'Stage MP cliff-escape common2 before/after status markers failed.' $gdbStdout
        $ce2f = Get-Ints $stageMPCliffEscapeCommon2Flags
        Assert-Condition ($stageMPCliffEscapeCommon2Flags.Success -and $ce2f[0] -ge 0 -and $ce2f[1] -eq $ce2f[0] -and $ce2f[2] -eq $ce2f[0] -and $ce2f[3] -eq $ce2f[1] -and $ce2f[4] -eq 1 -and $ce2f[5] -eq 1 -and $ce2f[6] -eq 1 -and $ce2f[7] -eq 0) 'Stage MP cliff-escape common2 floor/proc markers failed.' $gdbStdout
        $stageSummary = "$stageSummary mpCliffEscapeCommon2=status=$($ce2s[0])->$($ce2s[3])->$($ce2s[6])->$($ce2s[9]) motion=$($ce2s[1])->$($ce2s[4])->$($ce2s[7])->$($ce2s[10]) cliff=$($ce2f[0]) floor=$($ce2f[1])->$($ce2f[3]) calls=$($ce2c[2])/$($ce2c[5])/$($ce2c[7])"
    }
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after gcDrawAll proof.' $gdbStdout
    Write-Output ("$Label harness passed: scene=22/21 gcRunAll={0} gcDrawAll={1} display={2}/{3} draws={4} pixels={5} visits=0x{6:x}/0x{7:x} transitions=0x{8:x}/0x{9:x} screen-dx={10}/{11} screen-rise={12}/{13} final=Wait/Ground safe=1{14}" -f $tm[4], $tm[5], $dr[7], $dr[8], $dr[5], $dr[13], $st[10], $st[11], $st[12], $st[13], $sc[4], $sc[5], $sc[8], $sc[9], $stageSummary)
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
    if ($RunnerSlot -lt 0) {
        Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    }
}
