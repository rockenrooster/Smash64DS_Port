param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [string]$Harness = 'battle_mariofox_jump_loop',
    [string]$Target = 'smash64ds-battle-mariofox-jump-loop',
    [string]$Build = 'build-battle-mariofox-jump-loop-harness',
    [int]$ExpectedMode = 41,
    [int]$ExpectedHarnessSceneCurr = 22,
    [int]$ExpectedHarnessScenePrev = 21,
    [string]$Label = 'Battle Mario/Fox Jump-loop',
    [string]$HarnessSelectMessage = 'Direct Jump-loop harness did not select VSBattle from Maps.'
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
$target = $Target
$build = $Build
$rom = Join-Path $root "$target.nds"
$elf = Join-Path $root "$target.elf"
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir "melonds.$($Harness.Replace('_','-'))-harness.stdout.log"
$stderr = Join-Path $logDir "melonds.$($Harness.Replace('_','-'))-harness.stderr.log"
$configState = $null
$emulator = $null
$scriptName = "_$($Harness)_harness.gdb"
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=$target BUILD=$build NDS_DEV_SCENE_HARNESS=$Harness -j16
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
        'printf "WALK_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWalkLoopResult, gNdsFighterMarioFoxWalkLoopSafeResult, gNdsFighterMarioFoxWalkLoopMask, gNdsFighterMarioFoxWalkLoopDeferredMask, gNdsFighterMarioFoxWalkLoopCount',
        'printf "DASH_RUN=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDashRunResult, gNdsFighterMarioFoxDashRunSafeResult, gNdsFighterMarioFoxDashRunMask, gNdsFighterMarioFoxDashRunDeferredMask, gNdsFighterMarioFoxDashRunCount',
        'printf "JUMP_LOOP=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxJumpLoopResult, gNdsFighterMarioFoxJumpLoopSafeResult, gNdsFighterMarioFoxJumpLoopMask, gNdsFighterMarioFoxJumpLoopDeferredMask, gNdsFighterMarioFoxJumpLoopCount',
        'printf "JUMP_INPUT=%d,%d,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterJumpP0StickX, gNdsFighterJumpP1StickX, gNdsFighterJumpP0ButtonTap, gNdsFighterJumpP1ButtonTap, gNdsFighterJumpP0ButtonRelease, gNdsFighterJumpP1ButtonRelease, gNdsFighterJumpP0InputSource, gNdsFighterJumpP1InputSource, gNdsFighterJumpP0ShortHop, gNdsFighterJumpP1ShortHop',
        'printf "JUMP_STATUS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterJumpP0StatusStart, gNdsFighterJumpP1StatusStart, gNdsFighterJumpP0MotionStart, gNdsFighterJumpP1MotionStart, gNdsFighterJumpP0StatusWait, gNdsFighterJumpP1StatusWait, gNdsFighterJumpP0MotionWait, gNdsFighterJumpP1MotionWait, gNdsFighterJumpP0StatusKneeBend, gNdsFighterJumpP1StatusKneeBend, gNdsFighterJumpP0MotionKneeBend, gNdsFighterJumpP1MotionKneeBend, gNdsFighterJumpP0StatusJump, gNdsFighterJumpP1StatusJump, gNdsFighterJumpP0MotionJump, gNdsFighterJumpP1MotionJump',
        'printf "JUMP_GA=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterJumpP0GAStart, gNdsFighterJumpP1GAStart, gNdsFighterJumpP0GAWait, gNdsFighterJumpP1GAWait, gNdsFighterJumpP0GAKneeBend, gNdsFighterJumpP1GAKneeBend, gNdsFighterJumpP0GAJump, gNdsFighterJumpP1GAJump, gNdsFighterJumpP0GAAfterAir, gNdsFighterJumpP1GAAfterAir',
        'printf "JUMP_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterJumpRunBrakeEndCallCount, gNdsFighterJumpWaitSetStatusCount, gNdsFighterJumpWaitInterruptCallCount, gNdsFighterJumpGroundCheckCallCount, gNdsFighterJumpOriginalKneeBendCheckCallCount, gNdsFighterJumpOriginalKneeBendCheckSuccessCount, gNdsFighterJumpKneeBendSetStatusCallCount, gNdsFighterJumpFtMainKneeBendStatusCount, gNdsFighterJumpKneeBendUpdateCallCount, gNdsFighterJumpKneeBendInterruptCallCount, gNdsFighterJumpSetStatusCallCount, gNdsFighterJumpFtMainJumpStatusCount, gNdsFighterJumpSetAirCallCount, gNdsFighterJumpAirInterruptCallCount, gNdsFighterJumpAirPhysicsCallCount, gNdsFighterJumpAirMapCallCount',
        'printf "JUMP_FRAMES=%u,%u,%u,%u\n", gNdsFighterJumpP0KneeBendFrames, gNdsFighterJumpP1KneeBendFrames, gNdsFighterJumpP0AirFrames, gNdsFighterJumpP1AirFrames',
        'printf "JUMP_MOVE=%d,%d,%d,%d,%u,%u,%u,%u,%u\n", gNdsFighterJumpP0RootDeltaXMilli, gNdsFighterJumpP1RootDeltaXMilli, gNdsFighterJumpP0RootDeltaYMilli, gNdsFighterJumpP1RootDeltaYMilli, gNdsFighterJumpP0RootDirectionOK, gNdsFighterJumpP1RootDirectionOK, gNdsFighterJumpP0RootRiseOK, gNdsFighterJumpP1RootRiseOK, gNdsFighterJumpGObjDelta',
        'printf "JUMP_VEL=%d,%d,%d,%d,%d,%d,%d,%d\n", gNdsFighterJumpP0VelXInitialMilli, gNdsFighterJumpP1VelXInitialMilli, gNdsFighterJumpP0VelYInitialMilli, gNdsFighterJumpP1VelYInitialMilli, gNdsFighterJumpP0VelXAfterMilli, gNdsFighterJumpP1VelXAfterMilli, gNdsFighterJumpP0VelYAfterMilli, gNdsFighterJumpP1VelYAfterMilli',
        'printf "JUMP_ATTACKAIR=%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%u,%#x,%u,%#x,%#x,%#x,%#x,%#x\n", gNdsFighterJumpAttackAirCheckSuccessCount, gNdsFighterJumpAttackAirSetStatusCount, gNdsFighterJumpFtMainAttackAirStatusCount, gNdsFighterJumpAttackAirAnimEventsCount, gNdsFighterJumpAttackAirStatusAfter, gNdsFighterJumpAttackAirMotionAfter, gNdsFighterJumpAttackAirGAAfter, gNdsFighterJumpAttackAirMotionAttackIDAfter, gNdsFighterJumpAttackAirStatusAttackIDAfter, gNdsFighterJumpAttackAirStatAttackIDAfter, gNdsFighterJumpAttackAirTicsSinceLastZAfter, gNdsFighterJumpAttackAirCallbackMask, gNdsFighterJumpAttackAirRefreshCount, gNdsFighterJumpAttackAirRefreshMask, gNdsFighterJumpAttackAirRefreshStateMask, gNdsFighterJumpAttackAirRecordClearMask, gNdsFighterJumpAttackAirMapLandingMask, gNdsFighterJumpAttackAirDirectionMask',
        'printf "JUMP_DEFER=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterJumpDeferredInterruptCheckCount, gNdsFighterJumpSpecialHiCheckCount, gNdsFighterJumpAttackHi4KneeBendCheckCount, gNdsFighterJumpSpecialAirCheckCount, gNdsFighterJumpAttackAirCheckCount, gNdsFighterJumpAerialCheckCount, gNdsFighterJumpHammerHoldCheckCount, gNdsFighterJumpHammerKneeBendCheckCount, gNdsFighterJumpFallDeferredCount, gNdsFighterJumpLandingDeniedCount, gNdsFighterJumpCliffDeniedCount, gNdsFighterJumpCeilingDeniedCount',
        'printf "JUMP_SAFE=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterJumpDeniedStatusCount, gNdsFighterJumpUnexpectedStatusCount, gNdsFighterJumpProcessAttachCount, gNdsFighterJumpDisplayProbeCount, gNdsFighterJumpGameplayUpdateCount, gNdsFighterJumpDrawCallCount, gNdsFighterJumpMatrixCallCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $walkLoop = [regex]::Match($gdbStdout, 'WALK_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $dashRun = [regex]::Match($gdbStdout, 'DASH_RUN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $jumpLoop = [regex]::Match($gdbStdout, 'JUMP_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'JUMP_INPUT=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $status = [regex]::Match($gdbStdout, 'JUMP_STATUS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $ga = [regex]::Match($gdbStdout, 'JUMP_GA=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $calls = [regex]::Match($gdbStdout, 'JUMP_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $frames = [regex]::Match($gdbStdout, 'JUMP_FRAMES=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $move = [regex]::Match($gdbStdout, 'JUMP_MOVE=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $vel = [regex]::Match($gdbStdout, 'JUMP_VEL=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $attackAir = [regex]::Match($gdbStdout, 'JUMP_ATTACKAIR=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $defer = [regex]::Match($gdbStdout, 'JUMP_DEFER=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'JUMP_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq $ExpectedMode -and [int]$harn.Groups[3].Value -eq $ExpectedHarnessSceneCurr -and [int]$harn.Groups[4].Value -eq $ExpectedHarnessScenePrev -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) $HarnessSelectMessage $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($walkLoop.Success -and (Convert-MarkerUInt32 $walkLoop.Groups[1].Value) -eq 0x46574c50 -and (Convert-MarkerUInt32 $walkLoop.Groups[2].Value) -eq 0x46574c53 -and ((Convert-MarkerUInt32 $walkLoop.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $walkLoop.Groups[4].Value) -eq 0xff -and [int]$walkLoop.Groups[5].Value -eq 2) 'Prerequisite Walk-loop proof did not pass.' $gdbStdout
    Assert-Condition ($dashRun.Success -and (Convert-MarkerUInt32 $dashRun.Groups[1].Value) -eq 0x4644524e -and (Convert-MarkerUInt32 $dashRun.Groups[2].Value) -eq 0x46445253 -and ((Convert-MarkerUInt32 $dashRun.Groups[3].Value) -band 0x3ff) -eq 0x3ff -and (Convert-MarkerUInt32 $dashRun.Groups[4].Value) -eq 0xff -and [int]$dashRun.Groups[5].Value -eq 2) 'Prerequisite Dash-Run proof did not pass.' $gdbStdout
    Assert-Condition ($jumpLoop.Success -and (Convert-MarkerUInt32 $jumpLoop.Groups[1].Value) -eq 0x464a4d50 -and (Convert-MarkerUInt32 $jumpLoop.Groups[2].Value) -eq 0x464a4d53 -and ((Convert-MarkerUInt32 $jumpLoop.Groups[3].Value) -band 0xfff) -eq 0xfff -and (Convert-MarkerUInt32 $jumpLoop.Groups[4].Value) -eq 0xff -and [int]$jumpLoop.Groups[5].Value -eq 2) 'Jump-loop proof did not pass.' $gdbStdout
    Assert-Condition ($input.Success -and [int]$input.Groups[1].Value -ne 0 -and [int]$input.Groups[2].Value -ne 0 -and [int]$input.Groups[3].Value -eq 8 -and [int]$input.Groups[4].Value -eq 8 -and [int]$input.Groups[5].Value -eq 0 -and [int]$input.Groups[6].Value -eq 0 -and [int]$input.Groups[7].Value -eq 2 -and [int]$input.Groups[8].Value -eq 2 -and [int]$input.Groups[9].Value -eq 0 -and [int]$input.Groups[10].Value -eq 0) 'Jump input/source markers were not expected.' $gdbStdout
    Assert-Condition ($status.Success -and [int]$status.Groups[1].Value -eq 17 -and [int]$status.Groups[2].Value -eq 17 -and [int]$status.Groups[3].Value -eq 11 -and [int]$status.Groups[4].Value -eq 11 -and [int]$status.Groups[5].Value -eq 10 -and [int]$status.Groups[6].Value -eq 10 -and [int]$status.Groups[7].Value -eq 4 -and [int]$status.Groups[8].Value -eq 4 -and [int]$status.Groups[9].Value -eq 20 -and [int]$status.Groups[10].Value -eq 20 -and [int]$status.Groups[11].Value -eq 14 -and [int]$status.Groups[12].Value -eq 14 -and [int]$status.Groups[13].Value -eq 22 -and [int]$status.Groups[14].Value -eq 22 -and [int]$status.Groups[15].Value -eq 16 -and [int]$status.Groups[16].Value -eq 16) 'Jump status/motion path was not expected.' $gdbStdout
    Assert-Condition ($ga.Success -and [int]$ga.Groups[1].Value -eq 0 -and [int]$ga.Groups[2].Value -eq 0 -and [int]$ga.Groups[3].Value -eq 0 -and [int]$ga.Groups[4].Value -eq 0 -and [int]$ga.Groups[5].Value -eq 0 -and [int]$ga.Groups[6].Value -eq 0 -and [int]$ga.Groups[7].Value -eq 1 -and [int]$ga.Groups[8].Value -eq 1 -and [int]$ga.Groups[9].Value -eq 1 -and [int]$ga.Groups[10].Value -eq 1) 'Jump ground/air state markers were not expected.' $gdbStdout
    Assert-Condition ($calls.Success -and (@(1..8 | ForEach-Object { [int]$calls.Groups[$_].Value } | Where-Object { $_ -ne 2 }).Count -eq 0) -and [int]$calls.Groups[9].Value -ge 2 -and [int]$calls.Groups[10].Value -ge 2 -and [int]$calls.Groups[11].Value -eq 2 -and [int]$calls.Groups[12].Value -eq 2 -and [int]$calls.Groups[13].Value -eq 2 -and [int]$calls.Groups[14].Value -eq 12 -and [int]$calls.Groups[15].Value -eq 12 -and [int]$calls.Groups[16].Value -eq 12) 'Jump original call counters were not expected.' $gdbStdout
    Assert-Condition ($frames.Success -and [int]$frames.Groups[1].Value -ge 1 -and [int]$frames.Groups[2].Value -ge 1 -and [int]$frames.Groups[3].Value -eq 6 -and [int]$frames.Groups[4].Value -eq 6) 'Jump frame counters were not expected.' $gdbStdout
    Assert-Condition ($move.Success -and [int]$move.Groups[1].Value -ne 0 -and [int]$move.Groups[2].Value -ne 0 -and [int]$move.Groups[3].Value -gt 0 -and [int]$move.Groups[4].Value -gt 0 -and [int]$move.Groups[5].Value -eq 1 -and [int]$move.Groups[6].Value -eq 1 -and [int]$move.Groups[7].Value -eq 1 -and [int]$move.Groups[8].Value -eq 1 -and [int]$move.Groups[9].Value -eq 0) 'Jump root movement/safety markers failed.' $gdbStdout
    Assert-Condition ($vel.Success -and [int]$vel.Groups[3].Value -gt 0 -and [int]$vel.Groups[4].Value -gt 0 -and [int]$vel.Groups[7].Value -lt [int]$vel.Groups[3].Value -and [int]$vel.Groups[8].Value -lt [int]$vel.Groups[4].Value) 'Jump velocity markers were not expected.' $gdbStdout
    Assert-Condition ($attackAir.Success -and [int]$attackAir.Groups[1].Value -eq 1 -and [int]$attackAir.Groups[2].Value -eq 1 -and [int]$attackAir.Groups[3].Value -eq 1 -and [int]$attackAir.Groups[4].Value -eq 1 -and [int]$attackAir.Groups[5].Value -eq 209 -and [int]$attackAir.Groups[6].Value -eq 184 -and [int]$attackAir.Groups[7].Value -eq 1 -and [int]$attackAir.Groups[8].Value -eq 12 -and [int]$attackAir.Groups[9].Value -eq 9 -and [int]$attackAir.Groups[10].Value -eq 9 -and [int]$attackAir.Groups[11].Value -eq 65536 -and (Convert-MarkerUInt32 $attackAir.Groups[12].Value) -eq 0xf -and [int]$attackAir.Groups[13].Value -eq 2 -and (Convert-MarkerUInt32 $attackAir.Groups[14].Value) -eq 0x3 -and (Convert-MarkerUInt32 $attackAir.Groups[15].Value) -eq 0x7 -and (Convert-MarkerUInt32 $attackAir.Groups[16].Value) -eq 0x3 -and (Convert-MarkerUInt32 $attackAir.Groups[17].Value) -eq 0x3ff -and (Convert-MarkerUInt32 $attackAir.Groups[18].Value) -eq 0x1f) 'Jump AttackAirN marker was not expected.' $gdbStdout
    Assert-Condition ($defer.Success -and [int]$defer.Groups[1].Value -ge 24 -and [int]$defer.Groups[9].Value -eq 0 -and [int]$defer.Groups[10].Value -eq 0 -and [int]$defer.Groups[11].Value -eq 0 -and [int]$defer.Groups[12].Value -eq 0) 'Jump deferred/denied branch markers failed.' $gdbStdout
    Assert-Condition ($safe.Success -and (@(1..7 | ForEach-Object { [int]$safe.Groups[$_].Value } | Where-Object { $_ -ne 0 }).Count -eq 0)) 'Jump safe escape counters were not zero.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after Jump-loop proof.' $gdbStdout
    Write-Output ("$Label harness passed: jump dx={0}/{1} dy={2}/{3} vy={4}/{5}->{6}/{7} attackAir={8}/{9} map={10} dir={11}" -f $move.Groups[1].Value, $move.Groups[2].Value, $move.Groups[3].Value, $move.Groups[4].Value, $vel.Groups[3].Value, $vel.Groups[4].Value, $vel.Groups[7].Value, $vel.Groups[8].Value, $attackAir.Groups[5].Value, $attackAir.Groups[6].Value, $attackAir.Groups[17].Value, $attackAir.Groups[18].Value)
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
