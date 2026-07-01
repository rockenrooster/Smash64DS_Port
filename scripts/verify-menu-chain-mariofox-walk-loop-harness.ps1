param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
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
$rom = Join-Path $root 'smash64ds-menu-chain-mariofox-walk-loop.nds'
$elf = Join-Path $root 'smash64ds-menu-chain-mariofox-walk-loop.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.menu-chain-mariofox-walk-loop-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.menu-chain-mariofox-walk-loop-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_menu_chain_mariofox_walk_loop_harness.gdb'
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-menu-chain-mariofox-walk-loop BUILD=build-menu-chain-mariofox-walk-loop-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_walk_loop -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Menu-chain Mario/Fox Walk loop harness build did not produce the expected ROM and ELF.'
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
        'printf "CHAIN=%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask, gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask, gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask',
        'printf "CHAIN_FINAL=%u,%u,%u,%u,%u\n", gNdsVSModeStartTransitionSceneCurrFinal, gNdsPlayersVSReadyTransitionSceneCurrFinal, gNdsMapsSelectTransitionSceneCurrFinal, gNdsMapsSelectTransitionScenePrevFinal, gNdsMapsSelectTransitionSelectedGKind',
        'printf "FTR_DL_ALL=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLAllDrawResult, gNdsFighterMarioFoxDLAllDrawSafeResult, gNdsFighterMarioFoxDLAllDrawMask, gNdsFighterMarioFoxDLAllDrawDeferredMask, gNdsFighterMarioFoxDLAllDrawCount',
        'printf "FTR_WALK=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWalkInputResult, gNdsFighterMarioFoxWalkSafeResult, gNdsFighterMarioFoxWalkInputMask, gNdsFighterMarioFoxWalkDeferredMask, gNdsFighterMarioFoxWalkInputCount',
        'printf "FTR_WALK_LOOP=%#x,%#x,%#x,%#x,%u,%u\n", gNdsFighterMarioFoxWalkLoopResult, gNdsFighterMarioFoxWalkLoopSafeResult, gNdsFighterMarioFoxWalkLoopMask, gNdsFighterMarioFoxWalkLoopDeferredMask, gNdsFighterMarioFoxWalkLoopCount, gNdsFighterWalkLoopFrameTarget',
        'printf "FTR_WALK_LOOP_INPUT=%d,%d,%u,%u,%d,%d\n", gNdsFighterWalkLoopP0StickX, gNdsFighterWalkLoopP1StickX, gNdsFighterWalkLoopP0StickAbs, gNdsFighterWalkLoopP1StickAbs, gNdsFighterWalkLoopP0LR, gNdsFighterWalkLoopP1LR',
        'printf "FTR_WALK_LOOP_FRAMES=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWalkLoopP0HeldFrameCount, gNdsFighterWalkLoopP1HeldFrameCount, gNdsFighterWalkLoopP0InterruptCount, gNdsFighterWalkLoopP1InterruptCount, gNdsFighterWalkLoopP0PhysicsCount, gNdsFighterWalkLoopP1PhysicsCount, gNdsFighterWalkLoopP0IntegrateCount, gNdsFighterWalkLoopP1IntegrateCount, gNdsFighterWalkLoopP0MapCount, gNdsFighterWalkLoopP1MapCount, gNdsFighterWalkLoopP0SafeFloorCount, gNdsFighterWalkLoopP1SafeFloorCount',
        'printf "FTR_WALK_LOOP_STATE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWalkLoopP0StatusStart, gNdsFighterWalkLoopP1StatusStart, gNdsFighterWalkLoopP0StatusAfterHeld, gNdsFighterWalkLoopP1StatusAfterHeld, gNdsFighterWalkLoopP0StatusAfterRelease, gNdsFighterWalkLoopP1StatusAfterRelease, gNdsFighterWalkLoopP0StatusAfterSettle, gNdsFighterWalkLoopP1StatusAfterSettle, gNdsFighterWalkLoopP0MotionStart, gNdsFighterWalkLoopP1MotionStart, gNdsFighterWalkLoopP0MotionAfterHeld, gNdsFighterWalkLoopP1MotionAfterHeld, gNdsFighterWalkLoopP0MotionAfterRelease, gNdsFighterWalkLoopP1MotionAfterRelease, gNdsFighterWalkLoopP0MotionAfterSettle, gNdsFighterWalkLoopP1MotionAfterSettle',
        'printf "FTR_WALK_LOOP_MOVE=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%d,%d,%d,%d,%u,%u\n", gNdsFighterWalkLoopP0RootXStartBits, gNdsFighterWalkLoopP0RootXAfterHeldBits, gNdsFighterWalkLoopP0RootXAfterSettleBits, gNdsFighterWalkLoopP1RootXStartBits, gNdsFighterWalkLoopP1RootXAfterHeldBits, gNdsFighterWalkLoopP1RootXAfterSettleBits, gNdsFighterWalkLoopP0RootYStartBits, gNdsFighterWalkLoopP1RootYStartBits, gNdsFighterWalkLoopP0HeldRootDeltaXMilli, gNdsFighterWalkLoopP1HeldRootDeltaXMilli, gNdsFighterWalkLoopP0RootDeltaXMilli, gNdsFighterWalkLoopP1RootDeltaXMilli, gNdsFighterWalkLoopP0RootDirectionOK, gNdsFighterWalkLoopP1RootDirectionOK',
        'printf "FTR_WALK_LOOP_VEL=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\n", gNdsFighterWalkLoopP0GroundVelStartMilli, gNdsFighterWalkLoopP1GroundVelStartMilli, gNdsFighterWalkLoopP0GroundVelAfterHeldMilli, gNdsFighterWalkLoopP1GroundVelAfterHeldMilli, gNdsFighterWalkLoopP0GroundVelAfterSettleMilli, gNdsFighterWalkLoopP1GroundVelAfterSettleMilli, gNdsFighterWalkLoopP0AirVelXAfterHeldMilli, gNdsFighterWalkLoopP1AirVelXAfterHeldMilli, gNdsFighterWalkLoopP0AirVelYAfterHeldMilli, gNdsFighterWalkLoopP1AirVelYAfterHeldMilli, gNdsFighterWalkLoopGroundVelAbsStickCount, gNdsFighterWalkLoopGroundVelTransferAirCount, gNdsFighterWalkLoopWaitFrictionCount, gNdsFighterWalkLoopReleaseInputCount',
        'printf "FTR_WALK_LOOP_RELEASE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWalkLoopWaitReturnCheckCount, gNdsFighterWalkLoopWaitReturnSuccessCount, gNdsFighterWalkLoopWaitSetStatusCount, gNdsFighterWalkLoopP0GAAfterHeld, gNdsFighterWalkLoopP1GAAfterHeld, gNdsFighterWalkLoopP0GAAfterRelease, gNdsFighterWalkLoopP1GAAfterRelease, gNdsFighterWalkLoopP0GAAfterSettle, gNdsFighterWalkLoopP1GAAfterSettle, gNdsFighterWalkLoopMapSafeFloorCount',
        'printf "FTR_WALK_LOOP_SAFE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWalkLoopGObjDelta, gNdsFighterWalkLoopUnexpectedStatusCount, gNdsFighterWalkLoopDeniedStatusCount, gNdsFighterWalkLoopProcessAttachCount, gNdsFighterWalkLoopDisplayProbeCount, gNdsFighterWalkLoopGameplayUpdateCount, gNdsFighterWalkLoopDrawCallCount, gNdsFighterWalkLoopMatrixCallCount, gNdsFighterWalkLoopRootYDriftCount, gNdsFighterWalkLoopGADriftCount',
        'printf "FTR_WALK_LOOP_MAP=%u,%u,%u,%u,%u\n", gNdsFighterWalkLoopMapSafeFloorCount, gNdsFighterWalkLoopMapFallDeniedCount, gNdsFighterWalkLoopMapOttottoDeniedCount, gNdsFighterWalkLoopP0RootYStartBits == gNdsFighterWalkLoopP0RootYAfterSettleBits, gNdsFighterWalkLoopP1RootYStartBits == gNdsFighterWalkLoopP1RootYAfterSettleBits',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $chain = [regex]::Match($gdbStdout, 'CHAIN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $final = [regex]::Match($gdbStdout, 'CHAIN_FINAL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $dlAll = [regex]::Match($gdbStdout, 'FTR_DL_ALL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $walk = [regex]::Match($gdbStdout, 'FTR_WALK=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $loop = [regex]::Match($gdbStdout, 'FTR_WALK_LOOP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'FTR_WALK_LOOP_INPUT=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+)')
    $frames = [regex]::Match($gdbStdout, 'FTR_WALK_LOOP_FRAMES=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $state = [regex]::Match($gdbStdout, 'FTR_WALK_LOOP_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $move = [regex]::Match($gdbStdout, 'FTR_WALK_LOOP_MOVE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+)')
    $vel = [regex]::Match($gdbStdout, 'FTR_WALK_LOOP_VEL=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $release = [regex]::Match($gdbStdout, 'FTR_WALK_LOOP_RELEASE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'FTR_WALK_LOOP_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $map = [regex]::Match($gdbStdout, 'FTR_WALK_LOOP_MAP=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq 38 -and [int]$harn.Groups[3].Value -eq 9 -and [int]$harn.Groups[4].Value -eq 1 -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) 'Menu-chain Mario/Fox Walk loop harness did not select VS Mode from Title.' $gdbStdout
    Assert-Condition ($chain.Success -and (Convert-MarkerUInt32 $chain.Groups[1].Value) -eq 0x56535452 -and ((Convert-MarkerUInt32 $chain.Groups[2].Value) -band 0xff) -eq 0xff -and (Convert-MarkerUInt32 $chain.Groups[3].Value) -eq 0x50565452 -and ((Convert-MarkerUInt32 $chain.Groups[4].Value) -band 0xff) -eq 0xff -and (Convert-MarkerUInt32 $chain.Groups[5].Value) -eq 0x4d53454c -and ((Convert-MarkerUInt32 $chain.Groups[6].Value) -band 0xff) -eq 0xff) 'Menu-chain transitions did not reach Maps -> VSBattle cleanly.' $gdbStdout
    Assert-Condition ($final.Success -and [int]$final.Groups[1].Value -eq 16 -and [int]$final.Groups[2].Value -eq 21 -and [int]$final.Groups[3].Value -eq 22 -and [int]$final.Groups[4].Value -eq 21 -and [int]$final.Groups[5].Value -eq 6) 'Menu-chain final scene/gkind markers were not expected.' $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene state is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($dlAll.Success -and (Convert-MarkerUInt32 $dlAll.Groups[1].Value) -eq 0x4654414c -and (Convert-MarkerUInt32 $dlAll.Groups[2].Value) -eq 0x46544153 -and ((Convert-MarkerUInt32 $dlAll.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $dlAll.Groups[4].Value) -eq 0xff -and [int]$dlAll.Groups[5].Value -eq 2) 'Prerequisite all-DL proof did not pass.' $gdbStdout
    Assert-Condition ($walk.Success -and (Convert-MarkerUInt32 $walk.Groups[1].Value) -eq 0x4654574b -and (Convert-MarkerUInt32 $walk.Groups[2].Value) -eq 0x46545746 -and ((Convert-MarkerUInt32 $walk.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $walk.Groups[4].Value) -eq 0xff -and [int]$walk.Groups[5].Value -eq 2) 'Prerequisite Walk-input proof did not pass.' $gdbStdout
    Assert-Condition ($loop.Success -and (Convert-MarkerUInt32 $loop.Groups[1].Value) -eq 0x46574c50 -and (Convert-MarkerUInt32 $loop.Groups[2].Value) -eq 0x46574c53 -and ((Convert-MarkerUInt32 $loop.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $loop.Groups[4].Value) -eq 0xff -and [int]$loop.Groups[5].Value -eq 2 -and [int]$loop.Groups[6].Value -eq 4) 'Walk-loop proof did not pass.' $gdbStdout
    Assert-Condition ($input.Success -and [Math]::Abs([int]$input.Groups[1].Value) -eq 40 -and [Math]::Abs([int]$input.Groups[2].Value) -eq 80 -and [int]$input.Groups[3].Value -eq 40 -and [int]$input.Groups[4].Value -eq 80) 'Walk-loop held input was not expected.' $gdbStdout
    Assert-Condition ($frames.Success -and (@(1..12 | ForEach-Object { [int]$frames.Groups[$_].Value } | Where-Object { $_ -ne 4 }).Count -eq 0)) 'Walk-loop per-fighter frame callback counts were not exactly four.' $gdbStdout
    Assert-Condition ($state.Success -and [int]$state.Groups[1].Value -eq 12 -and [int]$state.Groups[2].Value -eq 13 -and [int]$state.Groups[3].Value -eq 12 -and [int]$state.Groups[4].Value -eq 13 -and [int]$state.Groups[5].Value -eq 10 -and [int]$state.Groups[6].Value -eq 10 -and [int]$state.Groups[7].Value -eq 10 -and [int]$state.Groups[8].Value -eq 10 -and [int]$state.Groups[9].Value -eq 6 -and [int]$state.Groups[10].Value -eq 7 -and [int]$state.Groups[11].Value -eq 6 -and [int]$state.Groups[12].Value -eq 7 -and [int]$state.Groups[13].Value -eq 4 -and [int]$state.Groups[14].Value -eq 4 -and [int]$state.Groups[15].Value -eq 4 -and [int]$state.Groups[16].Value -eq 4) 'Walk-loop state did not hold Walk then return to Wait.' $gdbStdout
    Assert-Condition ($move.Success -and (Convert-MarkerUInt32 $move.Groups[1].Value) -ne (Convert-MarkerUInt32 $move.Groups[2].Value) -and (Convert-MarkerUInt32 $move.Groups[4].Value) -ne (Convert-MarkerUInt32 $move.Groups[5].Value) -and [int]$move.Groups[9].Value -ne 0 -and [int]$move.Groups[10].Value -ne 0 -and [int]$move.Groups[11].Value -ne 0 -and [int]$move.Groups[12].Value -ne 0 -and [int]$move.Groups[13].Value -eq 1 -and [int]$move.Groups[14].Value -eq 1) 'Walk-loop root X did not move in facing direction.' $gdbStdout
    Assert-Condition ($vel.Success -and [int]$vel.Groups[1].Value -gt 0 -and [int]$vel.Groups[2].Value -gt 0 -and [int]$vel.Groups[3].Value -gt 0 -and [int]$vel.Groups[4].Value -gt 0 -and [int]$vel.Groups[5].Value -lt [int]$vel.Groups[3].Value -and [int]$vel.Groups[6].Value -lt [int]$vel.Groups[4].Value -and [int]$vel.Groups[7].Value -ne 0 -and [int]$vel.Groups[8].Value -ne 0 -and [int]$vel.Groups[9].Value -eq 0 -and [int]$vel.Groups[10].Value -eq 0 -and [int]$vel.Groups[11].Value -eq 8 -and [int]$vel.Groups[12].Value -ge 8 -and [int]$vel.Groups[13].Value -eq 2 -and [int]$vel.Groups[14].Value -eq 2) 'Walk-loop velocity/friction counters were not expected.' $gdbStdout
    Assert-Condition ($release.Success -and [int]$release.Groups[1].Value -eq 2 -and [int]$release.Groups[2].Value -eq 2 -and [int]$release.Groups[3].Value -eq 2 -and [int]$release.Groups[4].Value -eq 0 -and [int]$release.Groups[5].Value -eq 0 -and [int]$release.Groups[6].Value -eq 0 -and [int]$release.Groups[7].Value -eq 0 -and [int]$release.Groups[8].Value -eq 0 -and [int]$release.Groups[9].Value -eq 0 -and [int]$release.Groups[10].Value -ge 10) 'Walk-loop release/GA/safe-floor state was not expected.' $gdbStdout
    Assert-Condition ($map.Success -and [int]$map.Groups[1].Value -ge 10 -and [int]$map.Groups[2].Value -eq 0 -and [int]$map.Groups[3].Value -eq 0 -and [int]$map.Groups[4].Value -eq 1 -and [int]$map.Groups[5].Value -eq 1) 'Walk-loop map/root-Y stability failed.' $gdbStdout
    Assert-Condition ($safe.Success -and (@(1..10 | ForEach-Object { [int]$safe.Groups[$_].Value } | Where-Object { $_ -ne 0 }).Count -eq 0)) 'Walk-loop escaped bounded safety counters.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after Walk-loop proof.' $gdbStdout
    Write-Output ("Menu-chain Mario/Fox Walk loop harness passed: chain final=22/21 frames=4/4 root-dx={0}/{1} release=Wait vel={2}/{3}->{4}/{5} safe=1" -f $move.Groups[11].Value, $move.Groups[12].Value, $vel.Groups[3].Value, $vel.Groups[4].Value, $vel.Groups[5].Value, $vel.Groups[6].Value)
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
