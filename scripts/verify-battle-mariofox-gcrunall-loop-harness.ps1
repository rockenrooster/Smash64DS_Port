param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$ImportBattleShipFTManager,
    [switch]$ImportBattleShipBattlePlayable,
    [switch]$ImportBattleShipIFCommon,
    [switch]$ImportBattleShipMarioFireball,
    [switch]$ImportBattleShipFoxBlaster,
    [switch]$ImportBattleShipEffectManager,
    [switch]$ImportBattleShipFoxReflector,
    [switch]$ImportBattleShipMarioSpecialHi,
    [switch]$ImportBattleShipMarioSpecialLw,
    [switch]$ImportBattleShipFoxSpecialHi,
    [switch]$ImportBattleShipAudioAssets,
    [switch]$ImportBattleShipAudioBGM,
    [switch]$ImportBattleShipNormalMoveset,
    [switch]$HardwareTriangles,
    [switch]$BattlePlayable,
    [switch]$RealtimePresentation,
    [switch]$LiveInputPreview,
    [switch]$RequireRealtime60Fps,
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
$ImportBattleShipFTManager = $true
if ($BattlePlayable) {
    $ImportBattleShipBattlePlayable = $true
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
function Test-DamageDigits {
    param([int64]$Damage, [int64]$DigitCount, [int64]$DigitsPack)
    if ($Damage -le 0) { return $false }
    if ($Damage -gt 999) { $Damage = 999 }
    $damageText = [string]$Damage
    $expected = @(10)
    for ($i = $damageText.Length - 1; $i -ge 0; $i--) {
        $expected += [int]([string]$damageText[$i])
    }
    if ($DigitCount -ne $expected.Count) { return $false }
    for ($i = 0; $i -lt $expected.Count; $i++) {
        $digit = ($DigitsPack -shr (8 * $i)) -band 0xff
        if ($digit -ne $expected[$i]) { return $false }
    }
    return $true
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$makeArgs = @('-C', $root, "TARGET=$Target", "BUILD=$Build", "NDS_DEV_SCENE_HARNESS=$Harness", '-j16')
if ($ImportBattleShipFTManager) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FTMANAGER=1'
}
if ($ImportBattleShipBattlePlayable) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE=1'
}
if ($ImportBattleShipIFCommon) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_IFCOMMON=1'
}
if ($ImportBattleShipMarioFireball) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL=1'
}
if ($ImportBattleShipFoxBlaster) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FOX_BLASTER=1'
}
if ($ImportBattleShipEffectManager) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER=1'
}
if ($ImportBattleShipFoxReflector) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR=1'
}
if ($ImportBattleShipMarioSpecialHi) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI=1'
}
if ($ImportBattleShipMarioSpecialLw) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW=1'
}
if ($ImportBattleShipFoxSpecialHi) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI=1'
}
if ($ImportBattleShipAudioAssets) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS=1'
}
if ($ImportBattleShipAudioBGM) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_AUDIO_BGM=1'
}
if ($ImportBattleShipNormalMoveset) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET=1'
}
if ($HardwareTriangles) {
    $makeArgs += 'NDS_RENDERER_HW_TRIANGLES=1'
}
if ($LiveInputPreview) {
    $makeArgs += 'NDS_DEV_LIVE_INPUT_PREVIEW=1'
}
if ($RealtimePresentation) {
    $makeArgs += 'NDS_HARNESS_FAST_LOGIC=0'
} else {
    $makeArgs += 'NDS_HARNESS_FAST_LOGIC=1'
}
if (-not $NoBuild) {
    & make @makeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
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
    # The natural combat chain runs ~1000+ bounded updates; battle_playable
    # continues into an input-driven KO -> Rebirth -> Wait cycle.
    $minimumDelay = if ($BattlePlayable -and $RealtimePresentation) { 12 } elseif ($BattlePlayable) { 30 } else { 15 }
    Start-Sleep -Seconds ([Math]::Max($DelaySeconds, $minimumDelay))
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
        'printf "NAT_CHAIN=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterNaturalCombatPhase, gNdsFighterNaturalCombatPhaseFrames, gNdsFighterNaturalCombatStallCount, gNdsFighterNaturalCombatP0DashFrames, gNdsFighterNaturalCombatP1DashFrames, gNdsFighterNaturalCombatP0RunFrames, gNdsFighterNaturalCombatP1RunFrames, gNdsFighterNaturalCombatP0RunBrakeFrames, gNdsFighterNaturalCombatP1RunBrakeFrames, gNdsFighterNaturalCombatP0TurnFrames, gNdsFighterNaturalCombatP1TurnFrames, gNdsFighterNaturalCombatApproachDXMilli',
        'printf "NAT_ATTACK=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterNaturalCombatAttackerSlot, gNdsFighterNaturalCombatVictimSlot, gNdsFighterNaturalCombatAttackStatusFrames, gNdsFighterNaturalCombatAttackMotionFinal, gNdsFighterNaturalCombatHitboxActiveFrames, gNdsFighterNaturalCombatAttackRetryCount, gNdsFighterNaturalCombatVictimDamageStatus, gNdsFighterNaturalCombatVictimDamageFrames, gNdsFighterNaturalCombatVictimStartPercent, gNdsFighterNaturalCombatVictimFinalPercent, gNdsFighterNaturalCombatVictimKnockbackMilli, gNdsFighterNaturalCombatVictimRecoverWaitFrames',
        'printf "NAT_MOVESET=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%d,%u,%u\n", gNdsFighterNaturalMovesetMask, gNdsFighterNaturalMovesetPhase, gNdsFighterNaturalMovesetPhaseFrames, gNdsFighterNaturalMovesetTiltS3Frames, gNdsFighterNaturalMovesetTiltHi3Frames, gNdsFighterNaturalMovesetTiltLw3Frames, gNdsFighterNaturalMovesetTiltHitboxFrames, gNdsFighterNaturalMovesetSmashFrames, gNdsFighterNaturalMovesetSmashHitboxFrames, gNdsFighterNaturalMovesetAerialFrames, gNdsFighterNaturalMovesetAerialHitboxFrames, gNdsFighterNaturalMovesetLandingFrames, gNdsFighterNaturalMovesetCatchFrames, gNdsFighterNaturalMovesetCatchWaitFrames, gNdsFighterNaturalMovesetThrowFrames, gNdsFighterNaturalMovesetThrownFrames, gNdsFighterNaturalMovesetThrowRecoverFrames, gNdsFighterNaturalMovesetAttackerStatus, gNdsFighterNaturalMovesetAttackerMotion, gNdsFighterNaturalMovesetAttackerGA, gNdsFighterNaturalMovesetAttackerRootYMilli, gNdsFighterNaturalMovesetVictimStatus, gNdsFighterNaturalMovesetVictimMotion, gNdsFighterNaturalMovesetVictimGA, gNdsFighterNaturalMovesetVictimRootYMilli, gNdsFighterNaturalMovesetThrowDamageBefore, gNdsFighterNaturalMovesetThrowDamageAfter',
        'printf "NAT_HITBOX=%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%#x\n", gNdsFighterDashRunAttackEventLastPlayer, gNdsFighterDashRunAttackEventLastStatus, gNdsFighterDashRunAttackEventLastState, gNdsFighterDashRunAttackEventLastAttackID, gNdsFighterDashRunAttackEventLastGroupID, gNdsFighterDashRunAttackEventLastDamage, gNdsFighterDashRunAttackEventLastSize, gNdsFighterDashRunAttackEventLastOffsetX, gNdsFighterDashRunAttackEventLastOffsetY, gNdsFighterDashRunAttackEventLastOffsetZ, gNdsFighterDashRunAttackEventLastAngle, gNdsFighterDashRunAttackEventLastKBG, gNdsFighterDashRunAttackEventLastKBW, gNdsFighterDashRunAttackEventLastBKB, gNdsFighterDashRunAttackEventLastFlags',
        'printf "NAT_HITLAG=%u,%u\n", gNdsFighterNaturalCombatP0HitlagFrames, gNdsFighterNaturalCombatP1HitlagFrames',
        'printf "NAT_GUARD=%u,%u,%u\n", gNdsFighterNaturalCombatGuardOnFrames, gNdsFighterNaturalCombatGuardFrames, gNdsFighterNaturalCombatGuardOffFrames',
        'printf "BPLAY_GEOM=%u,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u\n", gNdsSCVSBattleStageGroundDataReady, (unsigned int)gMPCollisionGroundData, (unsigned int)gMPCollisionGeometry, (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->line_info : 0), (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->vertex_links : 0), (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->vertex_id : 0), (unsigned int)(gMPCollisionGeometry ? gMPCollisionGeometry->vertex_data : 0), gNdsPupupuGroundDeferredMask, gNdsStageCollisionLoopGeometryReady, gNdsStageCollisionLoopGroundDataReady, gNdsStageCollisionLoopFloorLineCount, gNdsStageCollisionLoopFloorLineMin, gNdsStageCollisionLoopFloorLineMaxExclusive, gNdsStageMPSweepFloorLoopLineSweepDiffCallCount, gNdsStageMPSweepFloorLoopLineSweepDiffHitCount, gNdsStageMPSweepFloorLoopLineSweepDiffMissCount, gNdsStageMPSweepFloorLoopLineSweepVisitCount, gNdsStageMPSweepFloorLoopLineSweepCandidateCount, gNdsStageMPSweepFloorLoopLineSweepSameHitCount',
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
        'printf "LIVE_PAD=%u,%u,%#x,%#x,%#x,%d,%d,%u,%u\n", gNdsControllerLiveReadCount, gNdsControllerLiveMapCount, gNdsControllerLiveConnectedMask, gNdsPlatformHeldKeys, gNdsControllerLivePad0Button, gNdsControllerLivePad0StickX, gNdsControllerLivePad0StickY, gNdsControllerPlaybackEnabled, gNdsControllerPlaybackReadCount',
        'detach',
        'quit'
    )
    if ($HardwareTriangles) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $hardwareCommands = @(
            'printf "PLATFORM_HW=%u,%u,%u,%u,%#x,%#x\n", gNdsHardwareRendererSubmittedFrameCount, gNdsHardwareRendererFlushCount, gNdsHardwareRendererPolyRamCount, gNdsHardwareRendererVertexRamCount, gNdsHardwareRendererStatus, gNdsHardwareRendererControl',
            'printf "STAGE_GCDRAWALL_HW=%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u\n", gNdsStageGCDrawAllLoopHardwareSubmitCount, gNdsStageGCDrawAllLoopHardwareTriangleCount, gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount, gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount, gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount, gNdsStageGCDrawAllLoopHardwareTextureBindCount, gNdsStageGCDrawAllLoopHardwareTextureUploadCount, gNdsStageGCDrawAllLoopHardwareTextureReadyCount, gNdsStageGCDrawAllLoopHardwareTextureRejectCount, gNdsStageGCDrawAllLoopHardwareTextureFormatMask, gNdsStageGCDrawAllLoopHardwareTextureMaxWidth, gNdsStageGCDrawAllLoopHardwareTextureMaxHeight',
            'printf "STAGE_GCDRAWALL_HW_FTR=%u,%u\n", gNdsStageGCDrawAllLoopHardwareFighterSubmitCount, gNdsStageGCDrawAllLoopHardwareFighterTriangleCount',
            'printf "RENDER_PROFILE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsRendererProfileFrameCount, gNdsRendererProfileUpdateTicks, gNdsRendererProfilePresentTicks, gNdsRendererProfileDrawTicks, gNdsRendererProfileHudTicks, gNdsRendererProfileStageAdapterTicks, gNdsRendererProfileMaterialTicks, gNdsRendererProfileMatrixTicks, gNdsRendererProfileDLTicks, gNdsRendererProfileTextureTicks, gNdsRendererProfileTextureConvertTicks, gNdsRendererProfileTextureUploadTicks, gNdsRendererProfileTextureUploads, gNdsRendererProfileTextureUploadBytes, gNdsRendererProfileTextureBinds, gNdsRendererProfileHardwareVertices, gNdsRendererProfileHardwareTriangles, gNdsRendererProfileHardwareOverLimit',
            'printf "RENDER_MATRIX=%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsRendererProfileMatrixLoadCount, gNdsRendererProfileMatrixScaleWorld, gNdsRendererProfileProjectionM00, gNdsRendererProfileProjectionM11, gNdsRendererProfileProjectionM22, gNdsRendererProfileProjectionM32, gNdsRendererProfileModelviewM00, gNdsRendererProfileModelviewM11, gNdsRendererProfileModelviewM22, gNdsRendererProfileModelviewM30, gNdsRendererProfileModelviewM31, gNdsRendererProfileModelviewM32',
            'printf "RENDER_VERTEX=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u\n", gNdsRendererProfileRawVertexMinX, gNdsRendererProfileRawVertexMaxX, gNdsRendererProfileRawVertexMinY, gNdsRendererProfileRawVertexMaxY, gNdsRendererProfileRawVertexMinZ, gNdsRendererProfileRawVertexMaxZ, gNdsRendererProfileHWVertexMinX, gNdsRendererProfileHWVertexMaxX, gNdsRendererProfileHWVertexMinY, gNdsRendererProfileHWVertexMaxY, gNdsRendererProfileHWVertexMinZ, gNdsRendererProfileHWVertexMaxZ, gNdsRendererProfileHWVertexSaturateCount'
        )
        $gdbCommands = @($beforeDetach + $hardwareCommands + $afterDetach)
    }
    if ($BattlePlayable) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $battlePlayableCommands = @(
            'printf "BPLAY_KO=%#x,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterBattlePlayableResult, gNdsFighterBattlePlayableMask, gNdsFighterBattlePlayableVictimSlot, gNdsFighterBattlePlayableVictimStockStart, gNdsFighterBattlePlayableVictimStockFinal, gNdsFighterBattlePlayableBattleStockStart, gNdsFighterBattlePlayableBattleStockFinal, gNdsFighterBattlePlayableFallsStart, gNdsFighterBattlePlayableFallsFinal',
            'printf "BPLAY_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterBattlePlayableDeadFrames, gNdsFighterBattlePlayableRebirthDownFrames, gNdsFighterBattlePlayableRebirthStandFrames, gNdsFighterBattlePlayableRebirthWaitFrames, gNdsFighterBattlePlayableFallAfterRebirthFrames, gNdsFighterBattlePlayableWaitAfterRebirthFrames, gNdsFighterBattlePlayableFinalStatus, gNdsFighterBattlePlayableFinalGA, gNdsFighterBattlePlayableFinalIsRebirth, gNdsFighterBattlePlayableKOStickFrames',
            'printf "BPLAY_MAP=%u,%u,%u,%u,%u,%#x,%#x,%d,%d,%d,%d,%d,%u,%u\n", gNdsFighterBattlePlayableMapCallCount, gNdsFighterBattlePlayableMapHitCount, gNdsFighterBattlePlayableMapFloorHitCount, gNdsFighterBattlePlayableMapCliffHitCount, gNdsFighterBattlePlayableMapCeilHitCount, gNdsFighterBattlePlayableMapLastMaskStat, gNdsFighterBattlePlayableMapLastMaskCurr, gNdsFighterBattlePlayableFinalXMilli, gNdsFighterBattlePlayableFinalYMilli, gNdsFighterBattlePlayableFinalVelXMilli, gNdsFighterBattlePlayableFinalVelYMilli, gNdsFighterBattlePlayableFinalFloorDistMilli, gNdsFighterBattlePlayableFinalFloor, gNdsFighterBattlePlayableFinalIsGhost',
            'printf "BPLAY_PACE=%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsBattlePlayablePacingResult, gNdsBattlePlayablePacingMode, gNdsBattlePlayablePacingLogicFrames, gNdsBattlePlayablePacingPresentedFrames, gNdsBattlePlayablePacingDrawCalls, gNdsBattlePlayablePacingTimerTicks, gNdsBattlePlayablePacingPresentFpsX10, gNdsBattlePlayablePacingLogicFpsX10',
            'printf "MEMARENA=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsMemoryLedgerResult, gNdsMemoryLedgerScene, gNdsMemoryLedgerGeneration, gNdsMemoryLedgerArenaCapacity, gNdsMemoryLedgerArenaUsed, gNdsMemoryLedgerArenaHighWater, gNdsMemoryLedgerArenaHeadroom, gNdsMemoryLedgerDLBytes, gNdsMemoryLedgerGraphicsBytes, gNdsMemoryLedgerRdpBytes, gNdsMemoryLedgerFigatreeHeapSize',
            'printf "MEMRELOC=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsMemoryLedgerRelocFiles, gNdsMemoryLedgerRelocBytes, gNdsMemoryLedgerRelocStageBytes, gNdsMemoryLedgerRelocFighterBytes, gNdsMemoryLedgerRelocInterfaceBytes, gNdsMemoryLedgerRelocMenuBytes, gNdsMemoryLedgerRelocOpeningBytes, gNdsMemoryLedgerRelocOtherBytes, gNdsMemoryLedgerRelocStaleFiles, gNdsMemoryLedgerRelocStaleBytes',
            'printf "MEMEVICT=%u,%u\n", gNdsMemoryLedgerEvictedFiles, gNdsMemoryLedgerEvictedBytes'
        )
        $gdbCommands = @($beforeDetach + $battlePlayableCommands + $afterDetach)
    }
    if ($ImportBattleShipIFCommon) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $hudCommands = @(
            'printf "IFHUD=%u,%#x,%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsIFCommonHUDRecordCount, gNdsIFCommonHUDObjectMask, gNdsIFCommonHUDP0DamageCurrent, gNdsIFCommonHUDP1DamageCurrent, gNdsIFCommonHUDP0DamageMax, gNdsIFCommonHUDP1DamageMax, gNdsIFCommonHUDP0DigitCount, gNdsIFCommonHUDP1DigitCount, gNdsIFCommonHUDP0Digits, gNdsIFCommonHUDP1Digits, gNdsIFCommonHUDP0StockCurrent, gNdsIFCommonHUDP1StockCurrent, gNdsIFCommonHUDP0StockMin, gNdsIFCommonHUDP1StockMin, gNdsIFCommonHUDP0StockMax, gNdsIFCommonHUDP1StockMax'
        )
        $gdbCommands = @($beforeDetach + $hudCommands + $afterDetach)
    }
    if ($ImportBattleShipMarioFireball -or $ImportBattleShipFoxBlaster) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $projectileCommands = @(
            'printf "PROJECTILE=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%#x,%u,%u,%#x\n", gNdsFighterProjectileProofResult, gNdsFighterProjectileProofMask, gNdsFighterProjectileProofActorSlot, gNdsFighterProjectileProofActorKind, gNdsFighterProjectileProofBPressFrames, gNdsFighterProjectileProofSpecialStatusFrames, gNdsFighterProjectileProofSpecialMotion, gNdsFighterProjectileProofAccessoryFrames, gNdsFighterProjectileProofFlag0Frames, gNdsFighterProjectileProofSpawnCallCount, gNdsFighterProjectileProofSpawnSuccessCount, gNdsFighterProjectileProofUpdateDestroyCount, gNdsFighterProjectileProofMapDestroyCount, gNdsFighterProjectileProofHitDestroyCount, gNdsFighterProjectileProofWeaponFrames, gNdsFighterProjectileProofWeaponCountMax, gNdsFighterProjectileProofKindMask, gNdsFighterProjectileProofAttackStateMask, gNdsFighterProjectileProofDamageMax, gNdsFighterProjectileProofLifetimeMax, gNdsFighterProjectileProofMapMask'
        )
        $gdbCommands = @($beforeDetach + $projectileCommands + $afterDetach)
    }
    if ($ImportBattleShipFoxReflector) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $reflectorCommands = @(
            'printf "REFLECTOR=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u,%u,%d,%d,%u,%u\n", gNdsFighterReflectorProofResult, gNdsFighterReflectorProofMask, gNdsFighterReflectorProofFoxSlot, gNdsFighterReflectorProofProjectileSlot, gNdsFighterReflectorProofDownBPressFrames, gNdsFighterReflectorProofStartFrames, gNdsFighterReflectorProofLoopFrames, gNdsFighterReflectorProofHitFrames, gNdsFighterReflectorProofIsReflectFrames, gNdsFighterReflectorProofReflectLRBeforeHit, gNdsFighterReflectorProofReflectLRClearFrames, gNdsFighterReflectorProofHitSetCallCount, gNdsFighterReflectorProofFireballProcCount, gNdsFighterReflectorProofFireballVelXBefore, gNdsFighterReflectorProofFireballVelXAfter, gNdsFighterReflectorProofFireballOwnerKind, gNdsFighterReflectorProofFireballCanReflect, gNdsFighterReflectorProofFireballCanAbsorb, gNdsFighterReflectorProofFireballCanShield, gNdsFighterReflectorProofFireballAttackCount, gNdsFighterReflectorProofFireballDamage, gNdsFighterReflectorProofFireballSizeMilli, gNdsFighterReflectorProofFireballDXMilli, gNdsFighterReflectorProofFireballDYMilli, gNdsFighterReflectorProofSpecialSizeMilli, gNdsFighterReflectorProofSpecialResist'
        )
        $gdbCommands = @($beforeDetach + $reflectorCommands + $afterDetach)
    }
    if ($ImportBattleShipMarioSpecialHi -or $ImportBattleShipMarioSpecialLw -or $ImportBattleShipFoxSpecialHi) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $specialsCommands = @(
            'printf "SPECIALS=%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d\n", gNdsFighterSpecialsProofMask, gNdsFighterSpecialsProofPhase, gNdsFighterSpecialsProofPhaseFrames, gNdsFighterSpecialsMarioSlot, gNdsFighterSpecialsFoxSlot, gNdsFighterSpecialsMarioHiPressFrames, gNdsFighterSpecialsMarioHiFrames, gNdsFighterSpecialsMarioAirHiFrames, gNdsFighterSpecialsMarioFallSpecialFrames, gNdsFighterSpecialsMarioLandingFallSpecialFrames, gNdsFighterSpecialsMarioHiWaitFrames, gNdsFighterSpecialsMarioHiRootYMilli, gNdsFighterSpecialsMarioLwPressFrames, gNdsFighterSpecialsMarioLwFrames, gNdsFighterSpecialsMarioAirLwFrames, gNdsFighterSpecialsMarioLwDustEffectCount, gNdsFighterSpecialsMarioLwWaitFrames, gNdsFighterSpecialsFoxHiPressFrames, gNdsFighterSpecialsFoxHiStartFrames, gNdsFighterSpecialsFoxHiHoldFrames, gNdsFighterSpecialsFoxHiTravelFrames, gNdsFighterSpecialsFoxHiEndFrames, gNdsFighterSpecialsFoxHiBoundFrames, gNdsFighterSpecialsFoxHiWaitFrames, gNdsFighterSpecialsFoxHiRootYMilli'
        )
        $gdbCommands = @($beforeDetach + $specialsCommands + $afterDetach)
    }
    if ($ImportBattleShipAudioAssets) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $audioCommands = @(
            'printf "AUDIO_ASSET=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioAssetResult, gNdsAudioAssetMask, gNdsAudioAssetOpenCount, gNdsAudioAssetOpenFailCount, gNdsAudioAssetFormatFailCount, gNdsAudioAssetShortReadCount, gNdsAudioAssetRawBytes, gNdsAudioAssetResidentBytes, gNdsAudioAssetScratchMaxBytes, gNdsAudioAssetSeqCount, gNdsAudioAssetSeqFirstOffset, gNdsAudioAssetSeqFirstLength, gNdsAudioAssetSeqMaxLength, gNdsAudioAssetBank1BankCount, gNdsAudioAssetBank1InstrumentCount, gNdsAudioAssetBank1WaveCount, gNdsAudioAssetBank1SampleRate, gNdsAudioAssetBank2BankCount, gNdsAudioAssetBank2InstrumentCount, gNdsAudioAssetBank2WaveCount, gNdsAudioAssetBank2SampleRate, gNdsAudioAssetFgmUnkCount, gNdsAudioAssetFgmTableCount, gNdsAudioAssetFgmUcodeCount'
        )
        $gdbCommands = @($beforeDetach + $audioCommands + $afterDetach)
    }
    if ($ImportBattleShipAudioBGM) {
        $beforeDetach = $gdbCommands[0..($gdbCommands.Count - 3)]
        $afterDetach = $gdbCommands[($gdbCommands.Count - 2)..($gdbCommands.Count - 1)]
        $audioBgmCommands = @(
            'printf "AUDIO_BGM=%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsAudioBgmResult, gNdsAudioBgmMask, gNdsAudioBgmPlaying, gNdsAudioBgmTrackID, gNdsAudioBgmVolume, gNdsAudioBgmPlayCalls, gNdsAudioBgmStopCalls, gNdsAudioBgmCheckCalls, gNdsAudioBgmSetVolumeCalls, gNdsAudioBgmOpenFailCount, gNdsAudioBgmReadFailCount, gNdsAudioBgmUnsupportedTrackCount, gNdsAudioBgmReadBytes, gNdsAudioBgmResidentBytes, gNdsAudioBgmChunkBytes, gNdsAudioBgmChunkPlayCount, gNdsAudioBgmStoppedOnTeardown, gNdsAudioBgmElapsedFrames, gNdsAudioBgmStreamedBytes, gNdsAudioBgmStreamBytesPerSecond, gNdsAudioBgmExpectedBytesPerSecond, gNdsAudioBgmLoopCount, gNdsAudioBgmRefillCount, gNdsAudioBgmPlaybackPositionBytes, gNdsAudioBgmWritePositionBytes, gNdsAudioBgmPlaybackHalf, gNdsAudioBgmWriteHalf, gNdsAudioBgmUnsafeWriteCount, gNdsAudioBgmTimerTicks, gNdsAudioBgmPlaybackBytes, gNdsAudioBgmPlaybackLoopCount, gNdsAudioBgmOverrunCount'
        )
        $gdbCommands = @($beforeDetach + $audioBgmCommands + $afterDetach)
    }
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
    $naturalChain = [regex]::Match($gdbStdout, 'NAT_CHAIN=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalAttack = [regex]::Match($gdbStdout, 'NAT_ATTACK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $naturalMoveset = [regex]::Match($gdbStdout, 'NAT_MOVESET=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $naturalHitlag = [regex]::Match($gdbStdout, 'NAT_HITLAG=([0-9]+),([0-9]+)')
    $naturalGuard = [regex]::Match($gdbStdout, 'NAT_GUARD=([0-9]+),([0-9]+),([0-9]+)')
    $battlePlayableKO = [regex]::Match($gdbStdout, 'BPLAY_KO=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $battlePlayableStatus = [regex]::Match($gdbStdout, 'BPLAY_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $battlePlayablePacing = [regex]::Match($gdbStdout, 'BPLAY_PACE=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $memoryArena = [regex]::Match($gdbStdout, 'MEMARENA=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $memoryReloc = [regex]::Match($gdbStdout, 'MEMRELOC=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $memoryEvict = [regex]::Match($gdbStdout, 'MEMEVICT=([0-9]+),([0-9]+)')
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
    $platformHw = [regex]::Match($gdbStdout, 'PLATFORM_HW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $stageHardware = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_HW=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $stageHardwareFighter = [regex]::Match($gdbStdout, 'STAGE_GCDRAWALL_HW_FTR=([0-9]+),([0-9]+)')
    $renderProfile = [regex]::Match($gdbStdout, 'RENDER_PROFILE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $renderMatrix = [regex]::Match($gdbStdout, 'RENDER_MATRIX=([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $renderVertex = [regex]::Match($gdbStdout, 'RENDER_VERTEX=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+)')
    $ifHud = [regex]::Match($gdbStdout, 'IFHUD=([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $projectile = [regex]::Match($gdbStdout, 'PROJECTILE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $reflector = [regex]::Match($gdbStdout, 'REFLECTOR=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $specials = [regex]::Match($gdbStdout, 'SPECIALS=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+)')
    $audioAsset = [regex]::Match($gdbStdout, 'AUDIO_ASSET=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $audioBgm = [regex]::Match($gdbStdout, 'AUDIO_BGM=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
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
        $nc = Get-Ints $naturalChain
        $na = Get-Ints $naturalAttack
        $nh = Get-Ints $naturalHitlag
        $ng = Get-Ints $naturalGuard
        if ($ImportBattleShipNormalMoveset) {
            $nm = Get-Ints $naturalMoveset
        }
        $hardwareSummary = ''
        if ($BattlePlayable -and $RealtimePresentation) {
            $bp = Get-Ints $battlePlayablePacing
            $minPresentedFrames = if ($RequireRealtime60Fps) { 60 } else { 45 }
            Assert-Condition ($battlePlayablePacing.Success -and $bp[0] -eq 0x42505443 -and $bp[1] -eq 0 -and (($bp[2] -eq $bp[3]) -or ($bp[2] -eq ($bp[3] + 1))) -and (($bp[4] -eq $bp[3]) -or ($bp[4] -eq ($bp[3] + 1))) -and $bp[3] -ge $minPresentedFrames -and $bp[5] -gt 0 -and $bp[6] -gt 0 -and $bp[7] -gt 0) 'battle_playable realtime pacing smoke did not present live frames or keep draw/update within one in-flight vblank.' $gdbStdout
            if ($RequireRealtime60Fps) {
                Assert-Condition ($bp[6] -ge 593 -and $bp[6] -le 603 -and $bp[7] -ge 593 -and $bp[7] -le 603) 'battle_playable realtime pacing failed 59.3..60.3 presented/logic fps.' $gdbStdout
            } elseif (($bp[6] -lt 593) -or ($bp[6] -gt 603) -or ($bp[7] -lt 593) -or ($bp[7] -gt 603)) {
                Write-Warning ("$Label realtime HW textured perf below 60fps target: fps=$($bp[6])/$($bp[7]) x0.1; renderer-cache follow-up still required.")
            }
            if ($HardwareTriangles) {
                $hw = Get-Ints $platformHw
                $shw = Get-Ints $stageHardware
                $shwf = Get-Ints $stageHardwareFighter
                $rp = Get-Ints $renderProfile
                $rm = Get-Ints $renderMatrix
                $rv = Get-Ints $renderVertex
                Assert-Condition ($platformHw.Success -and $hw[0] -gt 0 -and $hw[0] -eq $hw[1]) 'Canonical realtime HW build did not flush submitted DS 3D frames.' $gdbStdout
                Assert-Condition ($hw[2] -gt 0 -and $hw[3] -gt 0) 'Canonical realtime HW build submitted CPU-side triangles but DS GX polygon/vertex RAM stayed empty.' $gdbStdout
                Assert-Condition ($stageHardware.Success -and $shw[0] -gt 8 -and $shw[1] -gt 0 -and $shw[1] -eq ($shw[2] + $shw[3]) -and $shw[5] -gt 0 -and $shw[6] -gt 0 -and $shw[7] -gt 0 -and $shw[8] -eq 0) 'Canonical realtime HW build did not submit textured stage triangles.' $gdbStdout
                Assert-Condition ($stageHardwareFighter.Success -and $shwf[0] -ge 2 -and $shwf[1] -gt 0) 'Canonical realtime HW build did not submit fighter triangle sets.' $gdbStdout
                Assert-Condition ($renderProfile.Success -and $rp[15] -le 6144 -and $rp[16] -le 2048 -and $rp[17] -eq 0) 'Canonical realtime HW build exceeded DS poly/vertex limits.' $gdbStdout
                Assert-Condition ($renderMatrix.Success) 'Canonical realtime HW build did not report loaded GX matrix ranges.' $gdbStdout
                Assert-Condition ($renderVertex.Success) 'Canonical realtime HW build did not report submitted vertex ranges.' $gdbStdout
                $hardwareSummary = " gxram=$($hw[2])/$($hw[3]) gxstat=0x{0:x}/ctrl=0x{1:x} mtx=load$($rm[0])/scale$($rm[1])/p$($rm[2]),$($rm[3]),$($rm[4]),$($rm[5])/mv$($rm[6]),$($rm[7]),$($rm[8]),$($rm[9]),$($rm[10]),$($rm[11]) vraw=$($rv[0])..$($rv[1])/$($rv[2])..$($rv[3])/$($rv[4])..$($rv[5]) vhw=$($rv[6])..$($rv[7])/$($rv[8])..$($rv[9])/$($rv[10])..$($rv[11]) sat=$($rv[12]) profile=present$($rp[2])/draw$($rp[3])/stage$($rp[5])/mat$($rp[6])/mtx$($rp[7])/dl$($rp[8])/tex$($rp[9])/conv$($rp[10])/upload$($rp[11]) texUploads=$($rp[12])/$($rp[13]) binds=$($rp[14]) vtx=$($rp[15]) tri=$($rp[16])" -f $hw[4], $hw[5]
            }
            if ($LiveInputPreview) {
                $livePad = [regex]::Match($gdbStdout, 'LIVE_PAD=([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
                $lpv = Get-Ints $livePad
                Assert-Condition ($livePad.Success -and $lpv[0] -gt 0 -and $lpv[1] -gt 0 -and (($lpv[2] -band 1) -eq 1) -and $lpv[7] -eq 0 -and $lpv[8] -eq 0) 'Canonical realtime build did not use the live DS input path.' $gdbStdout
            }
            if ($ImportBattleShipAudioBGM) {
                $ab = Get-Ints $audioBgm
                Assert-Condition ($audioBgm.Success -and $ab[0] -eq 0x42474d31 -and (($ab[1] -band 0x1) -eq 0x1) -and (($ab[2] -eq 1) -or ($ab[6] -ge 1)) -and $ab[3] -eq 0 -and $ab[4] -eq 0x7800 -and $ab[5] -ge 1 -and $ab[9] -eq 0 -and $ab[10] -eq 0 -and $ab[11] -eq 0 -and $ab[13] -eq 65536 -and $ab[14] -eq 32768 -and $ab[19] -ge 42100 -and $ab[19] -le 46100 -and $ab[20] -eq 44100 -and $ab[22] -ge 4 -and $ab[23] -lt 65536 -and (($ab[24] -eq 0) -or ($ab[24] -eq 32768)) -and (($ab[25] -eq 0) -or ($ab[25] -eq 1)) -and (($ab[26] -eq 0) -or ($ab[26] -eq 1)) -and $ab[25] -ne $ab[26] -and $ab[27] -eq 0 -and $ab[28] -gt 0 -and $ab[29] -gt 0) 'Minimal BGM backend realtime smoke failed hardware-timer byte-rate or safe half refill guard.' $gdbStdout
            }
            Write-Output ("$Label realtime pacing smoke passed: frames=$($bp[3]) fps=$($bp[6])/$($bp[7]) ticks=$($bp[5])$hardwareSummary")
            return
        }
        $movementOnly = (($ExpectedMode -eq 39) -or ($ExpectedMode -eq 40))
        $liveHitOnly = (($ExpectedMode -eq 53) -or ($ExpectedMode -eq 54) -or ($ExpectedMode -eq 161) -or ($ExpectedMode -eq 162))
        $naturalMaskOk = if ($BattlePlayable) { (($nat[2] -band 0x6ffff) -eq 0x6ffff) } elseif ($movementOnly) { (($nat[2] -band 0x7fff) -eq 0x7fff) } elseif ($liveHitOnly) { (($nat[2] -band 0x3fdff) -eq 0x3fdff) } else { (($nat[2] -band 0xfffff) -eq 0xfffff) }
        $naturalAttackDamageOk = if ($BattlePlayable) { ($na[6] -gt 0 -and $na[7] -gt 0) } else { ($na[6] -gt 0 -and $na[7] -gt 0 -and $na[9] -gt $na[8]) }
        Assert-Condition ($natural.Success -and $nat[0] -eq 0x464e4d50 -and $nat[1] -eq 0x464e4d53 -and $naturalMaskOk -and $nat[3] -eq 1 -and $nat[4] -gt 0 -and $nat[5] -gt 0 -and $nat[6] -gt 0 -and $nat[7] -gt 0 -and (($nat[8] -band 0x3) -eq 0x3) -and $nat[10] -eq 0) 'Natural-motion manager runtime proof failed.' $gdbStdout
        Assert-Condition ($naturalFig.Success -and $nfig[0] -gt 0 -and $nfig[2] -eq 0 -and $nfig[3] -eq 0) 'Natural-motion figatree attach proof failed.' $gdbStdout
        Assert-Condition ($naturalWait.Success -and $nw[0] -ge 300 -and $nw[1] -ge 300 -and $nw[2] -gt 0 -and $nw[3] -gt 0 -and $nw[4] -ge 300 -and $nw[5] -ge 300) 'Natural-motion Wait animation proof failed.' $gdbStdout
        Assert-Condition ($naturalWalk.Success -and $nwalk[0] -gt 0 -and $nwalk[1] -ge 8 -and $nwalk[2] -ge 8 -and $nwalk[7] -gt 0 -and $nwalk[8] -gt 0 -and $nwalk[9] -gt 0 -and $nwalk[10] -gt 0) 'Natural-motion Walk transition proof failed.' $gdbStdout
        # Phase 14 == Done; phase 19 == battle_playable Done after KO/Rebirth.
        $expectedNaturalPhase = if ($BattlePlayable) { 19 } else { 14 }
        $naturalPhaseOk = if ($liveHitOnly) { ($nc[0] -ge 11) } else { ($nc[0] -eq $expectedNaturalPhase) }
        Assert-Condition ($naturalChain.Success -and $naturalPhaseOk -and $nc[2] -eq 0 -and $nc[3] -ge 2 -and $nc[4] -ge 2 -and $nc[5] -ge 8 -and $nc[6] -ge 8 -and $nc[7] -ge 2 -and $nc[8] -ge 2 -and $nc[9] -ge 1 -and $nc[10] -ge 1) 'Natural combat movement chain (dash/run/brake/turn) proof failed.' $gdbStdout
        if (-not $movementOnly) {
            $naturalRecoveryOk = if ($liveHitOnly) { $true } else { ($na[11] -gt 0) }
            Assert-Condition ($naturalAttack.Success -and $na[2] -gt 0 -and $na[4] -gt 0 -and $naturalAttackDamageOk -and $na[10] -gt 0 -and $naturalRecoveryOk) 'Natural attack->hit->damage lifecycle proof failed.' $gdbStdout
            Assert-Condition ($naturalHitlag.Success -and $nh[0] -gt 0 -and $nh[1] -gt 0) 'Natural hitlag proof failed on attacker/victim.' $gdbStdout
            if (-not $liveHitOnly) {
                Assert-Condition ($naturalGuard.Success -and $ng[0] -gt 0 -and $ng[1] -ge 10 -and $ng[2] -gt 0) 'Natural guard on/hold/off proof failed.' $gdbStdout
            }
        }
        $projectileSummary = ''
        if ($ImportBattleShipMarioFireball -or $ImportBattleShipFoxBlaster) {
            $pj = Get-Ints $projectile
            $expectedKind = if ($ImportBattleShipFoxReflector) { 0 } elseif ($ImportBattleShipFoxBlaster) { 1 } else { 0 }
            $projectileObserved = ($pj[14] -ge 3) -or ($pj[13] -gt 0)
            Assert-Condition ($projectile.Success -and $pj[0] -eq 0x50524f4a -and (($pj[1] -band 0x3f) -eq 0x3f) -and $pj[4] -gt 0 -and $pj[5] -gt 0 -and $projectileObserved -and $pj[15] -gt 0 -and (($pj[16] -band (1 -shl $expectedKind)) -ne 0) -and $pj[17] -ne 0 -and $pj[18] -gt 0) 'Natural projectile special proof failed.' $gdbStdout
            $projectileSummary = " projectile=actor$($pj[2])/kind$($pj[3]) b=$($pj[4]) status=$($pj[5]) accessory=$($pj[7]) flag0=$($pj[8]) spawn=$($pj[9]) ok=$($pj[10]) destroy=$($pj[11])/$($pj[12])/$($pj[13]) weaponFrames=$($pj[14]) max=$($pj[15]) kindMask=0x$('{0:x}' -f $pj[16]) attackMask=0x$('{0:x}' -f $pj[17]) dmg=$($pj[18]) life=$($pj[19]) map=0x$('{0:x}' -f $pj[20])"
        }
        if ($ImportBattleShipFoxReflector) {
            $rf = Get-Ints $reflector
            Assert-Condition ($reflector.Success -and $rf[0] -eq 0x52464c43 -and (($rf[1] -band 0xff) -eq 0xff) -and $rf[4] -gt 0 -and $rf[5] -gt 0 -and $rf[6] -gt 0 -and $rf[7] -gt 0 -and $rf[8] -gt 0 -and $rf[9] -ne 0 -and $rf[10] -gt 0 -and $rf[11] -gt 0 -and $rf[12] -gt 0 -and (($rf[13] -lt 0 -and $rf[14] -gt 0) -or ($rf[13] -gt 0 -and $rf[14] -lt 0)) -and $rf[15] -eq 1 -and $rf[16] -eq 1 -and $rf[19] -gt 0 -and $rf[20] -gt 0 -and $rf[21] -gt 0 -and $rf[24] -gt 0) 'Natural Fox reflector projectile proof failed.' $gdbStdout
            $projectileSummary += " reflector=0x$('{0:x}' -f $rf[1]) fox$($rf[2]) proj$($rf[3]) shine=$($rf[5])/$($rf[6])/$($rf[7]) reflect=$($rf[8]) lr=$($rf[9]) clear=$($rf[10]) proc=$($rf[12]) vx=$($rf[13])->$($rf[14]) owner=$($rf[15]) attrs=ref$($rf[16])/abs$($rf[17])/shield$($rf[18])/count$($rf[19])/dmg$($rf[20])/size$($rf[21]) delta=$($rf[22])/$($rf[23]) special=$($rf[24])/$($rf[25])"
        }
        $specialsSummary = ''
        if ($ImportBattleShipMarioSpecialHi -or $ImportBattleShipMarioSpecialLw -or $ImportBattleShipFoxSpecialHi) {
            $sp = Get-Ints $specials
            $expectedSpecialMask = 0
            if ($ImportBattleShipMarioSpecialHi) { $expectedSpecialMask = $expectedSpecialMask -bor 0x000f }
            if ($ImportBattleShipMarioSpecialLw) { $expectedSpecialMask = $expectedSpecialMask -bor 0x0070 }
            if ($ImportBattleShipFoxSpecialHi) { $expectedSpecialMask = $expectedSpecialMask -bor 0x0f80 }
            Assert-Condition ($specials.Success -and (($sp[0] -band $expectedSpecialMask) -eq $expectedSpecialMask) -and $sp[1] -eq 7) 'Natural remaining-specials proof failed.' $gdbStdout
            if ($ImportBattleShipMarioSpecialHi) {
                Assert-Condition ($sp[5] -gt 0 -and $sp[6] -gt 0 -and $sp[10] -ge 10 -and $sp[11] -gt 1000) 'Natural Mario Super Jump Punch status/launch/fall-special proof failed.' $gdbStdout
            }
            if ($ImportBattleShipMarioSpecialLw) {
                Assert-Condition ($sp[12] -gt 0 -and (($sp[13] -gt 0) -or ($sp[14] -gt 0)) -and $sp[15] -gt 0 -and $sp[16] -ge 10) 'Natural Mario Tornado status/effect/settle proof failed.' $gdbStdout
            }
            if ($ImportBattleShipFoxSpecialHi) {
                Assert-Condition ($sp[17] -gt 0 -and $sp[18] -gt 0 -and $sp[19] -gt 0 -and $sp[20] -gt 0 -and (($sp[21] -gt 0) -or ($sp[22] -gt 0)) -and $sp[23] -ge 10) 'Natural Fox Fire Fox status ladder proof failed.' $gdbStdout
            }
            $specialsSummary = " specials=0x$('{0:x}' -f $sp[0]) phase=$($sp[1]) mhi=$($sp[5])/$($sp[6])/$($sp[8])/$($sp[9])/$($sp[10]) y=$($sp[11]) mlw=$($sp[12])/$($sp[13])/$($sp[14]) dust=$($sp[15]) wait=$($sp[16]) foxhi=$($sp[17])/$($sp[18])/$($sp[19])/$($sp[20])/$($sp[21])/$($sp[22])/$($sp[23]) y=$($sp[24])"
        }
        $audioSummary = ''
        $audioBgmSummary = ''
        $audioBgmResidentBytes = 0
        if ($ImportBattleShipAudioAssets) {
            $aa = Get-Ints $audioAsset
            Assert-Condition ($audioAsset.Success -and $aa[0] -eq 0x41554431 -and $aa[1] -eq 0xff -and $aa[2] -eq 8 -and $aa[3] -eq 0 -and $aa[4] -eq 0 -and $aa[5] -eq 0 -and $aa[6] -eq 4422960 -and $aa[7] -eq 0 -and $aa[8] -le 65536 -and $aa[9] -eq 47 -and $aa[10] -eq 380 -and $aa[11] -eq 7999 -and $aa[12] -gt 0 -and $aa[13] -eq 1 -and $aa[14] -eq 42 -and $aa[15] -eq 117 -and $aa[16] -eq 32000 -and $aa[17] -eq 1 -and $aa[18] -eq 1 -and $aa[19] -eq 322 -and $aa[20] -eq 44100 -and $aa[21] -eq 100 -and $aa[22] -eq 464 -and $aa[23] -eq 695) 'Original audio asset parse-only proof failed.' $gdbStdout
            $audioSummary = " audio=seq$($aa[9]) bank1=$($aa[13])/$($aa[14])/$($aa[15])@$($aa[16]) bank2=$($aa[17])/$($aa[18])/$($aa[19])@$($aa[20]) fgm=$($aa[21])/$($aa[22])/$($aa[23]) raw=$($aa[6]) resident=$($aa[7]) scratch=$($aa[8])"
        }
        if ($ImportBattleShipAudioBGM) {
            $ab = Get-Ints $audioBgm
            $audioBgmResidentBytes = $ab[13]
            Assert-Condition ($audioBgm.Success -and $ab[0] -eq 0x42474d31 -and (($ab[1] -band 0x3) -eq 0x3) -and $ab[2] -eq 0 -and $ab[3] -eq 0 -and $ab[4] -eq 0x7800 -and $ab[5] -ge 1 -and $ab[6] -ge 1 -and $ab[9] -eq 0 -and $ab[10] -eq 0 -and $ab[11] -eq 0 -and $ab[12] -gt 65536 -and $ab[13] -eq 65536 -and $ab[14] -eq 32768 -and $ab[15] -ge 1 -and $ab[16] -eq 1 -and $ab[17] -ge 3200 -and $ab[19] -ge 42100 -and $ab[19] -le 46100 -and $ab[20] -eq 44100 -and $ab[22] -ge 4 -and $ab[23] -lt 65536 -and (($ab[24] -eq 0) -or ($ab[24] -eq 32768)) -and (($ab[25] -eq 0) -or ($ab[25] -eq 1)) -and (($ab[26] -eq 0) -or ($ab[26] -eq 1)) -and $ab[25] -ne $ab[26] -and $ab[27] -eq 0 -and $ab[28] -gt 0 -and $ab[29] -gt 0 -and $ab[31] -eq 0) 'Minimal BGM backend proof failed natural start/stop or hardware-timer 44100 B/s stream-rate guard.' $gdbStdout
            $audioBgmSummary = " bgm=track$($ab[3]) play=$($ab[5]) stop=$($ab[6]) refills=$($ab[22]) read=$($ab[12]) rate=$($ab[19]) loop=$($ab[21]) hwloop=$($ab[30]) resident=$($ab[13])"
        }
        $movesetSummary = ''
        if ($ImportBattleShipNormalMoveset) {
            Assert-Condition ($naturalMoveset.Success -and (($nm[0] -band 0x7ff) -eq 0x7ff) -and $nm[1] -eq 15 -and $nm[3] -gt 0 -and $nm[4] -gt 0 -and $nm[5] -gt 0 -and $nm[6] -gt 0 -and $nm[7] -gt 0 -and $nm[8] -gt 0 -and $nm[9] -gt 0 -and $nm[11] -gt 0 -and $nm[12] -gt 0 -and $nm[13] -gt 0 -and $nm[14] -gt 0 -and $nm[15] -gt 0 -and $nm[16] -ge 10 -and $nm[26] -gt $nm[25]) 'Natural normal-moveset tilt/smash/aerial/grab/throw proof failed.' $gdbStdout
            $movesetSummary = " moveset=0x$('{0:x}' -f $nm[0]) phase=$($nm[1]) tilt=$($nm[3])/$($nm[4])/$($nm[5]) smash=$($nm[7]) aerial=$($nm[9]) landing=$($nm[11]) grab=$($nm[12])/$($nm[13]) throw=$($nm[14])/$($nm[15])/$($nm[16]) throwDmg=$($nm[25])->$($nm[26])"
        }
        $battlePlayableSummary = ''
        if ($BattlePlayable) {
            $bpk = Get-Ints $battlePlayableKO
            $bps = Get-Ints $battlePlayableStatus
            $ma = Get-Ints $memoryArena
            $mr = Get-Ints $memoryReloc
            $me = Get-Ints $memoryEvict
            $bp = Get-Ints $battlePlayablePacing
            $victimStockDelta = $bpk[3] - $bpk[4]
            $battleStockDelta = $bpk[5] - $bpk[6]
            $fallsDelta = $bpk[8] - $bpk[7]
            Assert-Condition ($battlePlayablePacing.Success -and $bp[0] -eq 0x42505443 -and $bp[1] -eq 1 -and $bp[2] -ge 3200 -and $bp[3] -eq 0 -and $bp[4] -eq 0 -and $bp[5] -gt 0) 'battle_playable fast-logic pacing marker did not prove explicit unthrottled verification mode.' $gdbStdout
            Assert-Condition ($battlePlayableKO.Success -and $bpk[0] -eq 0x42504c59 -and (($bpk[1] -band 0xff) -eq 0xff) -and $victimStockDelta -gt 0 -and $victimStockDelta -eq $battleStockDelta -and $victimStockDelta -eq $fallsDelta) 'battle_playable stock/fall KO proof failed.' $gdbStdout
            Assert-Condition ($battlePlayableStatus.Success -and $bps[0] -gt 0 -and $bps[1] -gt 0 -and $bps[2] -gt 0 -and $bps[3] -gt 0 -and $bps[5] -ge 8 -and $bps[6] -eq 10 -and $bps[7] -eq 0 -and $bps[8] -eq 0 -and $bps[9] -gt 0) 'battle_playable Dead/Rebirth/return-control proof failed.' $gdbStdout
            $interfaceBytesOk = if ($ImportBattleShipIFCommon) { $mr[4] -gt 0 } else { $true }
            Assert-Condition ($memoryArena.Success -and $ma[0] -eq 0x4d4c4544 -and $ma[1] -eq 22 -and $ma[3] -ge 0x130000 -and ($ma[6] - $audioBgmResidentBytes) -ge 131072 -and $ma[7] -eq 163840 -and $ma[8] -eq 106496 -and $ma[9] -eq 49152 -and $ma[10] -gt 0) 'battle_playable memory arena ledger failed reserve or VSBattle taskman buffer assertions.' $gdbStdout
            Assert-Condition ($memoryReloc.Success -and $mr[0] -gt 0 -and $mr[1] -gt 0 -and $mr[2] -gt 0 -and $mr[3] -gt 0 -and $interfaceBytesOk -and $mr[5] -eq 0 -and $mr[6] -eq 0 -and $mr[8] -eq 0 -and $mr[9] -eq 0) 'battle_playable reloc memory ledger found stale or missing resident groups.' $gdbStdout
            Assert-Condition ($memoryEvict.Success) 'battle_playable reloc eviction ledger was not printed.' $gdbStdout
            $battlePlayableSummary = " bplay=stock$($bpk[3])->$($bpk[4]) falls$($bpk[7])->$($bpk[8]) dead=$($bps[0]) rebirth=$($bps[1])/$($bps[2])/$($bps[3]) recover=$($bps[4])/$($bps[5])"
            $battlePlayableSummary += " pace=fast logic$($bp[2]) draw$($bp[4])"
            $battlePlayableSummary += " mem=head$($ma[6]) reloc$($mr[1]) stage$($mr[2]) fighter$($mr[3]) if$($mr[4]) stale$($mr[8])/$($mr[9]) evict$($me[0])/$($me[1])"
            $battlePlayableSummary += $movesetSummary
            $battlePlayableSummary += $specialsSummary
            $battlePlayableSummary += $audioSummary
            $battlePlayableSummary += $audioBgmSummary
            if ($ImportBattleShipIFCommon) {
                $ih = Get-Ints $ifHud
                $victimSlot = [int]$bpk[2]
                $damageMax = @($ih[4], $ih[5])
                $digitCounts = @($ih[6], $ih[7])
                $digitPacks = @($ih[8], $ih[9])
                $stockCurrent = @($ih[10], $ih[11])
                $stockMin = @($ih[12], $ih[13])
                $stockMax = @($ih[14], $ih[15])
                $stockDelta = $stockMax[$victimSlot] - $stockMin[$victimSlot]
                $stockCurrentMatchesFinal = ($stockCurrent[$victimSlot] -eq $bpk[4]) -or ($stockCurrent[$victimSlot] -eq ($bpk[4] + 1))
                Assert-Condition ($ifHud.Success -and $ih[0] -gt 0 -and (($ih[1] -band 0x33) -eq 0x33)) 'Imported IFCommon HUD objects were not observed for both players.' $gdbStdout
                Assert-Condition ((Test-DamageDigits -Damage $damageMax[$victimSlot] -DigitCount $digitCounts[$victimSlot] -DigitsPack $digitPacks[$victimSlot])) 'Imported IFCommon percent digit images did not match the victim damage value.' $gdbStdout
                Assert-Condition ($stockMax[$victimSlot] -ge $bpk[3] -and $stockDelta -ge $victimStockDelta -and $stockCurrentMatchesFinal) 'Imported IFCommon stock icon display did not decrement after KO.' $gdbStdout
                $battlePlayableSummary += " hud=dmg$($damageMax[$victimSlot])/digits0x$('{0:x}' -f $digitPacks[$victimSlot]) stock$($stockMax[$victimSlot])->$($stockMin[$victimSlot])"
            }
        }
        if ($HardwareTriangles) {
            $hw = Get-Ints $platformHw
            Assert-Condition ($platformHw.Success -and $hw[0] -gt 0 -and $hw[0] -eq $hw[1]) 'Boundary hardware draw did not flush submitted DS 3D frames.' $gdbStdout
            $shw = Get-Ints $stageHardware
            Assert-Condition ($stageHardware.Success -and $shw[0] -gt 8) 'Boundary hardware replay did not exceed the old bounded DObj submit slice.' $gdbStdout
            Assert-Condition ($shw[1] -gt 0) 'Boundary hardware replay did not submit hardware triangles.' $gdbStdout
            Assert-Condition ($shw[1] -eq ($shw[2] + $shw[3])) 'Boundary hardware depth accounting does not match submitted triangles.' $gdbStdout
            Assert-Condition ($shw[3] -gt 0) 'Boundary hardware replay did not preserve source no-z projected-depth submission.' $gdbStdout
            Assert-Condition ($shw[5] -gt 0 -and $shw[6] -gt 0 -and $shw[7] -gt 0 -and $shw[9] -ne 0 -and $shw[10] -gt 0 -and $shw[11] -gt 0) 'Boundary hardware replay did not bind/upload a ready texture.' $gdbStdout
            Assert-Condition ($shw[8] -eq 0) 'Boundary hardware replay still rejects source-loaded stage texture state.' $gdbStdout
            $shwf = Get-Ints $stageHardwareFighter
            Assert-Condition ($stageHardwareFighter.Success -and $shwf[0] -eq 2 -and $shwf[1] -gt 0) 'Boundary hardware replay did not submit selected fighter triangles.' $gdbStdout
            $hardwareSummary = " hwflush=$($hw[0])/$($hw[1]) hwsubmit=$($shw[0]) hwtri=$($shw[1]) hwdepth=z$($shw[2])/proj$($shw[3])/decal$($shw[4]) hwtex=bind$($shw[5])/upload$($shw[6])/ready$($shw[7])/reject$($shw[8])/fmt$($shw[9])/max$($shw[10])x$($shw[11]) hwftr=$($shwf[0])/$($shwf[1])"
        }
        Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after natural-motion proof.' $gdbStdout
        Write-Output ("$Label ftmanager natural-combat harness passed: wait={0}/{1} walk={2}/{3} dash={4}/{5} run={6}/{7} attack={8} hitbox={9} dmg={10}->{11} status={12} knock={13} guard={14}/{15}/{16} retries={17} updates={18} mask=0x{19:x}{20}{21}{22}" -f $nw[0], $nw[1], $nwalk[1], $nwalk[2], $nc[3], $nc[4], $nc[5], $nc[6], $na[2], $na[4], $na[8], $na[9], $na[6], $na[10], $ng[0], $ng[1], $ng[2], $na[5], $nat[4], $nat[2], $projectileSummary, $battlePlayableSummary, $hardwareSummary)
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
