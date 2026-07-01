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
$rom = Join-Path $root 'smash64ds-battle-mariofox-walk-input.nds'
$elf = Join-Path $root 'smash64ds-battle-mariofox-walk-input.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-walk-input-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-walk-input-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_walk_input_harness.gdb'
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-mariofox-walk-input BUILD=build-battle-mariofox-walk-input-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_walk_input -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox Walk input harness build did not produce the expected ROM and ELF.'
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
        'printf "FTR_DL_ALL=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDLAllDrawResult, gNdsFighterMarioFoxDLAllDrawSafeResult, gNdsFighterMarioFoxDLAllDrawMask, gNdsFighterMarioFoxDLAllDrawDeferredMask, gNdsFighterMarioFoxDLAllDrawCount',
        'printf "FTR_DL_ALL_COUNTS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDLAllDrawP0CandidateCount, gNdsFighterDLAllDrawP1CandidateCount, gNdsFighterDLAllDrawP0SelectedCount, gNdsFighterDLAllDrawP1SelectedCount, gNdsFighterDLAllDrawP0CleanCount, gNdsFighterDLAllDrawP1CleanCount, gNdsFighterDLAllDrawP0FailedCount, gNdsFighterDLAllDrawP1FailedCount, gNdsFighterDLAllDrawP0PixelCount, gNdsFighterDLAllDrawP1PixelCount',
        'printf "FTR_DL_ALL_GEOM=%u,%u,%u,%u\n", gNdsFighterDLAllDrawP0TriangleCount, gNdsFighterDLAllDrawP1TriangleCount, gNdsFighterDLAllDrawP0TriangleDrawnCount, gNdsFighterDLAllDrawP1TriangleDrawnCount',
        'printf "FTR_DL_ALL_SAFE=%u,%u,%u,%u,%u,%u\n", gNdsFighterDLAllDrawGObjDelta, gNdsFighterDLAllDrawDrawCallCount, gNdsFighterDLAllDrawMatrixCallCount, gNdsFighterDLAllDrawGameplayUpdateCount, gNdsFighterDLAllDrawRangeRejectCount, gNdsFighterDLAllDrawVertexRangeRejectCount',
        'printf "FTR_WALK=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWalkInputResult, gNdsFighterMarioFoxWalkSafeResult, gNdsFighterMarioFoxWalkInputMask, gNdsFighterMarioFoxWalkDeferredMask, gNdsFighterMarioFoxWalkInputCount',
        'printf "FTR_WALK_INPUT=%d,%d,%u,%u,%d,%d,%u,%u,%u,%u\n", gNdsFighterWalkP0StickX, gNdsFighterWalkP1StickX, gNdsFighterWalkP0StickAbs, gNdsFighterWalkP1StickAbs, gNdsFighterWalkP0LR, gNdsFighterWalkP1LR, gNdsFighterWalkP0InputSuccess, gNdsFighterWalkP1InputSuccess, gNdsFighterWalkP0SelectedStatus, gNdsFighterWalkP1SelectedStatus',
        'printf "FTR_WALK_STATE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWalkP0StatusBefore, gNdsFighterWalkP1StatusBefore, gNdsFighterWalkP0StatusAfter, gNdsFighterWalkP1StatusAfter, gNdsFighterWalkP0MotionBefore, gNdsFighterWalkP1MotionBefore, gNdsFighterWalkP0MotionAfter, gNdsFighterWalkP1MotionAfter, gNdsFighterWalkP0GABefore, gNdsFighterWalkP1GABefore, gNdsFighterWalkP0GAAfter, gNdsFighterWalkP1GAAfter',
        'printf "FTR_WALK_CALLBACKS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWalkWaitInterruptCallCount, gNdsFighterWalkGroundCheckCallCount, gNdsFighterWalkOriginalCheckCallCount, gNdsFighterWalkOriginalCheckSuccessCount, gNdsFighterWalkSetStatusCallCount, gNdsFighterWalkFtMainSetStatusCallCount, gNdsFighterWalkAnimEventsCallCount, gNdsFighterWalkCallbackReadyCount, gNdsFighterWalkLoopInterruptCallCount, gNdsFighterWalkDeferredInterruptCheckCount',
        'printf "FTR_WALK_PHYS=%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWalkP0GroundVelBeforeMilli, gNdsFighterWalkP1GroundVelBeforeMilli, gNdsFighterWalkP0GroundVelAfterMilli, gNdsFighterWalkP1GroundVelAfterMilli, gNdsFighterWalkP0AirVelXMilli, gNdsFighterWalkP1AirVelXMilli, gNdsFighterWalkP0AirVelYMilli, gNdsFighterWalkP1AirVelYMilli, gNdsFighterWalkGroundVelAbsStickCount, gNdsFighterWalkGroundVelTransferAirCount, gNdsFighterWalkPhysicsCallbackCount, gNdsFighterWalkMapCallbackCount, gNdsFighterWalkMapSafeFloorCount, gNdsFighterWalkMapFallDeniedCount, gNdsFighterWalkMapOttottoDeniedCount',
        'printf "FTR_WALK_ROOT=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFighterWalkP0RootXBeforeBits, gNdsFighterWalkP0RootXAfterBits, gNdsFighterWalkP0RootYBeforeBits, gNdsFighterWalkP0RootYAfterBits, gNdsFighterWalkP1RootXBeforeBits, gNdsFighterWalkP1RootXAfterBits, gNdsFighterWalkP1RootYBeforeBits, gNdsFighterWalkP1RootYAfterBits',
        'printf "FTR_WALK_SAFE=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWalkGObjDelta, gNdsFighterWalkDeniedStatusCount, gNdsFighterWalkUnexpectedStatusCount, gNdsFighterWalkProcessAttachCount, gNdsFighterWalkDisplayProbeCount, gNdsFighterWalkGameplayUpdateCount, gNdsFighterWalkDrawCallCount, gNdsFighterWalkMatrixCallCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $all = [regex]::Match($gdbStdout, 'FTR_DL_ALL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $counts = [regex]::Match($gdbStdout, 'FTR_DL_ALL_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $geom = [regex]::Match($gdbStdout, 'FTR_DL_ALL_GEOM=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $allSafe = [regex]::Match($gdbStdout, 'FTR_DL_ALL_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $walk = [regex]::Match($gdbStdout, 'FTR_WALK=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $input = [regex]::Match($gdbStdout, 'FTR_WALK_INPUT=(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $state = [regex]::Match($gdbStdout, 'FTR_WALK_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $callbacks = [regex]::Match($gdbStdout, 'FTR_WALK_CALLBACKS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $phys = [regex]::Match($gdbStdout, 'FTR_WALK_PHYS=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),(-?[0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $rootMark = [regex]::Match($gdbStdout, 'FTR_WALK_ROOT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $safe = [regex]::Match($gdbStdout, 'FTR_WALK_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq 35 -and [int]$harn.Groups[3].Value -eq 22 -and [int]$harn.Groups[4].Value -eq 21 -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) 'Battle Mario/Fox Walk input harness did not select direct VSBattle from Maps.' $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene state is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($all.Success -and (Convert-MarkerUInt32 $all.Groups[1].Value) -eq 0x4654414c -and (Convert-MarkerUInt32 $all.Groups[2].Value) -eq 0x46544153 -and ((Convert-MarkerUInt32 $all.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $all.Groups[4].Value) -eq 0xff -and [int]$all.Groups[5].Value -eq 2) 'Prerequisite all-DL proof did not pass.' $gdbStdout
    Assert-Condition ($counts.Success -and [int]$counts.Groups[1].Value -eq 14 -and [int]$counts.Groups[2].Value -eq 18 -and [int]$counts.Groups[3].Value -eq 14 -and [int]$counts.Groups[4].Value -eq 18 -and [int]$counts.Groups[5].Value -eq 14 -and [int]$counts.Groups[6].Value -ge 17 -and [int]$counts.Groups[7].Value -eq 0 -and [int]$counts.Groups[8].Value -le 1 -and ([int]$counts.Groups[6].Value + [int]$counts.Groups[8].Value) -eq [int]$counts.Groups[4].Value -and [int]$counts.Groups[9].Value -eq 14913 -and [int]$counts.Groups[10].Value -eq 13432) 'Prerequisite all-DL counts changed.' $gdbStdout
    Assert-Condition ($geom.Success -and [int]$geom.Groups[1].Value -eq 334 -and [int]$geom.Groups[2].Value -eq 322 -and [int]$geom.Groups[3].Value -gt 0 -and [int]$geom.Groups[4].Value -gt 0) 'Prerequisite all-DL geometry changed.' $gdbStdout
    Assert-Condition ($allSafe.Success -and [int]$allSafe.Groups[1].Value -eq 0 -and [int]$allSafe.Groups[2].Value -eq 0 -and [int]$allSafe.Groups[3].Value -eq 0 -and [int]$allSafe.Groups[4].Value -eq 0 -and [int]$allSafe.Groups[5].Value -eq 0 -and [int]$allSafe.Groups[6].Value -eq 0) 'Prerequisite all-DL proof escaped bounded behavior.' $gdbStdout
    Assert-Condition ($walk.Success -and (Convert-MarkerUInt32 $walk.Groups[1].Value) -eq 0x4654574b -and (Convert-MarkerUInt32 $walk.Groups[2].Value) -eq 0x46545746 -and ((Convert-MarkerUInt32 $walk.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $walk.Groups[4].Value) -eq 0xff -and [int]$walk.Groups[5].Value -eq 2) 'Mario/Fox Walk input proof did not pass.' $gdbStdout
    Assert-Condition ($input.Success -and [Math]::Abs([int]$input.Groups[1].Value) -eq 40 -and [Math]::Abs([int]$input.Groups[2].Value) -eq 80 -and [int]$input.Groups[3].Value -eq 40 -and [int]$input.Groups[4].Value -eq 80 -and [int]$input.Groups[7].Value -eq 1 -and [int]$input.Groups[8].Value -eq 1 -and [int]$input.Groups[9].Value -eq 12 -and [int]$input.Groups[10].Value -eq 13) 'Walk synthetic input/status selection was not expected.' $gdbStdout
    Assert-Condition ($state.Success -and [int]$state.Groups[1].Value -eq 10 -and [int]$state.Groups[2].Value -eq 10 -and [int]$state.Groups[3].Value -eq 12 -and [int]$state.Groups[4].Value -eq 13 -and [int]$state.Groups[5].Value -eq 4 -and [int]$state.Groups[6].Value -eq 4 -and [int]$state.Groups[7].Value -eq 6 -and [int]$state.Groups[8].Value -eq 7 -and [int]$state.Groups[9].Value -eq 0 -and [int]$state.Groups[10].Value -eq 0 -and [int]$state.Groups[11].Value -eq 0 -and [int]$state.Groups[12].Value -eq 0) 'Walk state transition did not match Wait -> WalkMiddle/WalkFast.' $gdbStdout
    Assert-Condition ($callbacks.Success -and [int]$callbacks.Groups[1].Value -eq 2 -and [int]$callbacks.Groups[2].Value -eq 2 -and [int]$callbacks.Groups[3].Value -eq 2 -and [int]$callbacks.Groups[4].Value -eq 2 -and [int]$callbacks.Groups[5].Value -eq 2 -and [int]$callbacks.Groups[6].Value -eq 2 -and [int]$callbacks.Groups[7].Value -eq 2 -and [int]$callbacks.Groups[8].Value -eq 2 -and [int]$callbacks.Groups[9].Value -eq 2 -and [int]$callbacks.Groups[10].Value -gt 0) 'Walk callbacks did not run through the expected bounded original path.' $gdbStdout
    Assert-Condition ($phys.Success -and [int]$phys.Groups[1].Value -eq 0 -and [int]$phys.Groups[2].Value -eq 0 -and [int]$phys.Groups[3].Value -gt 0 -and [int]$phys.Groups[4].Value -gt 0 -and [int]$phys.Groups[9].Value -eq 2 -and [int]$phys.Groups[10].Value -eq 2 -and [int]$phys.Groups[11].Value -eq 2 -and [int]$phys.Groups[12].Value -eq 2 -and [int]$phys.Groups[13].Value -eq 2 -and [int]$phys.Groups[14].Value -eq 0 -and [int]$phys.Groups[15].Value -eq 0) 'Walk physics/map proof did not generate safe nonzero ground velocity.' $gdbStdout
    Assert-Condition ($rootMark.Success -and (Convert-MarkerUInt32 $rootMark.Groups[1].Value) -eq (Convert-MarkerUInt32 $rootMark.Groups[2].Value) -and (Convert-MarkerUInt32 $rootMark.Groups[3].Value) -eq (Convert-MarkerUInt32 $rootMark.Groups[4].Value) -and (Convert-MarkerUInt32 $rootMark.Groups[5].Value) -eq (Convert-MarkerUInt32 $rootMark.Groups[6].Value) -and (Convert-MarkerUInt32 $rootMark.Groups[7].Value) -eq (Convert-MarkerUInt32 $rootMark.Groups[8].Value)) 'Walk proof moved fighter root positions.' $gdbStdout
    Assert-Condition ($safe.Success -and [int]$safe.Groups[1].Value -eq 0 -and [int]$safe.Groups[2].Value -eq 0 -and [int]$safe.Groups[3].Value -eq 0 -and [int]$safe.Groups[4].Value -eq 0 -and [int]$safe.Groups[5].Value -eq 0 -and [int]$safe.Groups[6].Value -eq 0 -and [int]$safe.Groups[7].Value -eq 0 -and [int]$safe.Groups[8].Value -eq 0) 'Walk proof escaped bounded safety counters.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary after Walk input proof.' $gdbStdout
    Write-Output ("Battle Mario/Fox Walk input harness passed: scene=22/21 status=12/13 motion=6/7 stick=40/80 vel={0}/{1} callbacks=2 safe=1" -f $phys.Groups[3].Value, $phys.Groups[4].Value)
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
