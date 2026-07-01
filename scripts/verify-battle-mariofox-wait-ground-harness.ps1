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
$rom = Join-Path $root 'smash64ds-battle-mariofox-wait-ground.nds'
$elf = Join-Path $root 'smash64ds-battle-mariofox-wait-ground.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-wait-ground-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-wait-ground-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_wait_ground_harness.gdb'
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-mariofox-wait-ground BUILD=build-battle-mariofox-wait-ground-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_ground -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox Wait ground harness build did not produce the expected ROM and ELF.'
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
        'printf "VSB=%#x,%#x,%#x,%u,%u,%u\n", gNdsSCVSBattleOriginalSetupResult, gNdsSCVSBattleOriginalSetupMask, gNdsSCVSBattleOriginalUpdateResult, gNdsSCVSBattleOriginalPlayerCount, gNdsSCVSBattleOriginalFighterCreateCount, gNdsSCVSBattleOriginalGKind',
        'printf "STAGE=%#x,%#x,%#x,%#x,%u\n", gNdsSCVSBattleStageResult, gNdsSCVSBattleStageMask, gNdsPupupuGroundSetupResult, gNdsPupupuGroundSetupMask, gNdsPupupuGroundMapGObjCount',
        'printf "FTR_BASE=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxModelResult, gNdsFighterMarioFoxGObjResult, gNdsFighterMarioFoxStructResult, gNdsFighterMarioFoxJointResult, gNdsFighterMarioFoxStateResult, gNdsFighterMarioFoxInitResult, gNdsFighterMarioFoxCollResult, gNdsFighterModelRealGObjCount, gNdsFighterMarioFoxStructCount, gNdsFighterMarioFoxInitCount',
        'printf "FTR_WAIT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWaitStatusResult, gNdsFighterMarioFoxWaitMotionResult, gNdsFighterMarioFoxWaitDeferResult, gNdsFighterMarioFoxWaitMask, gNdsFighterMarioFoxWaitDeferredMask, gNdsFighterMarioFoxWaitCount',
        'printf "FTR_TICK=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWaitTickResult, gNdsFighterMarioFoxWaitCallbackResult, gNdsFighterMarioFoxWaitSafeResult, gNdsFighterMarioFoxWaitTickMask, gNdsFighterMarioFoxWaitTickDeferredMask, gNdsFighterMarioFoxWaitTickCount',
        'printf "FTR_GROUND=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxGroundPhysResult, gNdsFighterMarioFoxGroundMapResult, gNdsFighterMarioFoxGroundSafeResult, gNdsFighterMarioFoxGroundMask, gNdsFighterMarioFoxGroundDeferredMask, gNdsFighterMarioFoxGroundCount',
        'printf "FTR_GROUND_VEL=%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitGroundP0VelBeforeMilli, gNdsFighterWaitGroundP0VelAfterMilli, gNdsFighterWaitGroundP1VelBeforeMilli, gNdsFighterWaitGroundP1VelAfterMilli, gNdsFighterWaitGroundP0AirVelXMilli, gNdsFighterWaitGroundP1AirVelXMilli, gNdsFighterWaitGroundP0AirVelYMilli, gNdsFighterWaitGroundP1AirVelYMilli',
        'printf "FTR_GROUND_MAT=%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitGroundP0Material, gNdsFighterWaitGroundP1Material, gNdsFighterWaitGroundP0TractionMilli, gNdsFighterWaitGroundP1TractionMilli, gNdsFighterWaitGroundP0FrictionMilli, gNdsFighterWaitGroundP1FrictionMilli',
        'printf "FTR_GROUND_STATE=%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitGroundP0StatusAfter, gNdsFighterWaitGroundP1StatusAfter, gNdsFighterWaitGroundP0MotionAfter, gNdsFighterWaitGroundP1MotionAfter, gNdsFighterWaitGroundP0GAAfter, gNdsFighterWaitGroundP1GAAfter',
        'printf "FTR_GROUND_ROOT=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFighterWaitGroundP0RootXBeforeBits, gNdsFighterWaitGroundP0RootXAfterBits, gNdsFighterWaitGroundP0RootYBeforeBits, gNdsFighterWaitGroundP0RootYAfterBits, gNdsFighterWaitGroundP1RootXBeforeBits, gNdsFighterWaitGroundP1RootXAfterBits, gNdsFighterWaitGroundP1RootYBeforeBits, gNdsFighterWaitGroundP1RootYAfterBits',
        'printf "FTR_GROUND_CALLS=%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitGroundPhysicsCallbackCount, gNdsFighterWaitGroundMapCallbackCount, gNdsFighterWaitGroundMapCheckCount, gNdsFighterWaitGroundMapSafeFloorCount, gNdsFighterWaitGroundMapFallDeniedCount, gNdsFighterWaitGroundMapOttottoDeniedCount',
        'printf "FTR_GROUND_SAFE=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitGroundStatusChangeCount, gNdsFighterWaitGroundMotionChangeCount, gNdsFighterWaitGroundGADriftCount, gNdsFighterWaitGroundRootDriftCount, gNdsFighterWaitGroundGObjDelta, gNdsFighterWaitGroundDisplayProbeCount, gNdsFighterWaitGroundGameplayUpdateCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $vsb = [regex]::Match($gdbStdout, 'VSB=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $stage = [regex]::Match($gdbStdout, 'STAGE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $base = [regex]::Match($gdbStdout, 'FTR_BASE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $wait = [regex]::Match($gdbStdout, 'FTR_WAIT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $tick = [regex]::Match($gdbStdout, 'FTR_TICK=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $ground = [regex]::Match($gdbStdout, 'FTR_GROUND=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $vel = [regex]::Match($gdbStdout, 'FTR_GROUND_VEL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $mat = [regex]::Match($gdbStdout, 'FTR_GROUND_MAT=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $state = [regex]::Match($gdbStdout, 'FTR_GROUND_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $root = [regex]::Match($gdbStdout, 'FTR_GROUND_ROOT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $calls = [regex]::Match($gdbStdout, 'FTR_GROUND_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'FTR_GROUND_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq 21 -and [int]$harn.Groups[3].Value -eq 22 -and [int]$harn.Groups[4].Value -eq 21 -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) 'Battle Mario/Fox Wait ground harness did not select direct VSBattle from Maps.' $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene state is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($vsb.Success -and (Convert-MarkerUInt32 $vsb.Groups[1].Value) -eq 0x56425355 -and ((Convert-MarkerUInt32 $vsb.Groups[2].Value) -band 0x7f) -eq 0x7f -and (Convert-MarkerUInt32 $vsb.Groups[3].Value) -eq 0x56425550 -and [int]$vsb.Groups[4].Value -eq 2 -and [int]$vsb.Groups[5].Value -eq 2 -and [int]$vsb.Groups[6].Value -eq 6) 'Imported VSBattle setup did not reach the ground proof boundary.' $gdbStdout
    Assert-Condition ($stage.Success -and (Convert-MarkerUInt32 $stage.Groups[1].Value) -eq 0x50555042 -and ((Convert-MarkerUInt32 $stage.Groups[2].Value) -band 0xff) -eq 0xff -and (Convert-MarkerUInt32 $stage.Groups[3].Value) -eq 0x50554753 -and ((Convert-MarkerUInt32 $stage.Groups[4].Value) -band 0x3ff) -eq 0x3ff -and [int]$stage.Groups[5].Value -eq 4) 'Pupupu stage/ground setup failed before ground proof.' $gdbStdout
    Assert-Condition ($base.Success -and (Convert-MarkerUInt32 $base.Groups[1].Value) -eq 0x46544d44 -and (Convert-MarkerUInt32 $base.Groups[2].Value) -eq 0x4654474f -and (Convert-MarkerUInt32 $base.Groups[3].Value) -eq 0x46545348 -and (Convert-MarkerUInt32 $base.Groups[4].Value) -eq 0x46544a54 -and (Convert-MarkerUInt32 $base.Groups[5].Value) -eq 0x46545354 -and (Convert-MarkerUInt32 $base.Groups[6].Value) -eq 0x4654494e -and (Convert-MarkerUInt32 $base.Groups[7].Value) -eq 0x4654434c -and [int]$base.Groups[8].Value -eq 2 -and [int]$base.Groups[9].Value -eq 2 -and [int]$base.Groups[10].Value -eq 2) 'Mario/Fox model/struct/init base proof failed before ground proof.' $gdbStdout
    Assert-Condition ($wait.Success -and (Convert-MarkerUInt32 $wait.Groups[1].Value) -eq 0x46545753 -and (Convert-MarkerUInt32 $wait.Groups[2].Value) -eq 0x4654574d -and (Convert-MarkerUInt32 $wait.Groups[3].Value) -eq 0x46545744 -and ((Convert-MarkerUInt32 $wait.Groups[4].Value) -band 0xfff) -eq 0xfff -and (Convert-MarkerUInt32 $wait.Groups[5].Value) -eq 0xff -and [int]$wait.Groups[6].Value -eq 2) 'Mario/Fox Wait setup proof failed before ground proof.' $gdbStdout
    Assert-Condition ($tick.Success -and (Convert-MarkerUInt32 $tick.Groups[1].Value) -eq 0x4654544b -and (Convert-MarkerUInt32 $tick.Groups[2].Value) -eq 0x46544342 -and (Convert-MarkerUInt32 $tick.Groups[3].Value) -eq 0x46545346 -and ((Convert-MarkerUInt32 $tick.Groups[4].Value) -band 0x3ff) -eq 0x3ff -and (Convert-MarkerUInt32 $tick.Groups[5].Value) -eq 0xff -and [int]$tick.Groups[6].Value -eq 2) 'Mario/Fox Wait tick proof failed before ground proof.' $gdbStdout
    Assert-Condition ($ground.Success -and (Convert-MarkerUInt32 $ground.Groups[1].Value) -eq 0x46544750 -and (Convert-MarkerUInt32 $ground.Groups[2].Value) -eq 0x4654474d -and (Convert-MarkerUInt32 $ground.Groups[3].Value) -eq 0x46544753 -and ((Convert-MarkerUInt32 $ground.Groups[4].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $ground.Groups[5].Value) -eq 0xff -and [int]$ground.Groups[6].Value -eq 2) 'Mario/Fox Wait ground proof did not pass.' $gdbStdout
    $p0Before = [int]$vel.Groups[1].Value
    $p0After = [int]$vel.Groups[2].Value
    $p1Before = [int]$vel.Groups[3].Value
    $p1After = [int]$vel.Groups[4].Value
    Assert-Condition ($vel.Success -and $p0Before -eq 2000 -and $p1Before -eq 2000 -and $p0After -lt $p0Before -and $p1After -lt $p1Before -and $p0After -ge 0 -and $p1After -ge 0 -and [int]$vel.Groups[7].Value -eq 0 -and [int]$vel.Groups[8].Value -eq 0) 'Ground velocity/friction transfer values are incorrect.' $gdbStdout
    Assert-Condition ($mat.Success -and [int]$mat.Groups[5].Value -gt 0 -and [int]$mat.Groups[6].Value -gt 0) 'Ground material friction diagnostics were not populated.' $gdbStdout
    Assert-Condition ($state.Success -and [int]$state.Groups[1].Value -eq 10 -and [int]$state.Groups[2].Value -eq 10 -and [int]$state.Groups[3].Value -eq 4 -and [int]$state.Groups[4].Value -eq 4 -and [int]$state.Groups[5].Value -eq 0 -and [int]$state.Groups[6].Value -eq 0) 'Ground proof changed fighter status, motion, or GA state.' $gdbStdout
    Assert-Condition ($root.Success -and (Convert-MarkerUInt32 $root.Groups[1].Value) -eq (Convert-MarkerUInt32 $root.Groups[2].Value) -and (Convert-MarkerUInt32 $root.Groups[3].Value) -eq (Convert-MarkerUInt32 $root.Groups[4].Value) -and (Convert-MarkerUInt32 $root.Groups[5].Value) -eq (Convert-MarkerUInt32 $root.Groups[6].Value) -and (Convert-MarkerUInt32 $root.Groups[7].Value) -eq (Convert-MarkerUInt32 $root.Groups[8].Value)) 'Root DObj position drifted during ground proof.' $gdbStdout
    Assert-Condition ($calls.Success -and [int]$calls.Groups[1].Value -eq 2 -and [int]$calls.Groups[2].Value -eq 2 -and [int]$calls.Groups[3].Value -eq 2 -and [int]$calls.Groups[4].Value -eq 2 -and [int]$calls.Groups[5].Value -eq 0 -and [int]$calls.Groups[6].Value -eq 0) 'Ground proof callback/map safety counters failed.' $gdbStdout
    Assert-Condition ($safe.Success -and [int]$safe.Groups[1].Value -eq 0 -and [int]$safe.Groups[2].Value -eq 0 -and [int]$safe.Groups[3].Value -eq 0 -and [int]$safe.Groups[4].Value -eq 0 -and [int]$safe.Groups[5].Value -eq 0 -and [int]$safe.Groups[6].Value -eq 0 -and [int]$safe.Groups[7].Value -eq 0) 'Ground proof escaped the bounded safety contract.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary.' $gdbStdout
    Write-Output ("Battle Mario/Fox Wait ground harness passed: scene=22/21 ground={0} vel={1}->{2}/{3}->{4} map=2/2 stable=1" -f $ground.Groups[4].Value, $p0Before, $p0After, $p1Before, $p1After)
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
