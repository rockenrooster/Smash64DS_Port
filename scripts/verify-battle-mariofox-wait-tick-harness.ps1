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
$rom = Join-Path $root 'smash64ds-battle-mariofox-wait-tick.nds'
$elf = Join-Path $root 'smash64ds-battle-mariofox-wait-tick.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-wait-tick-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-wait-tick-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_wait_tick_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-mariofox-wait-tick BUILD=build-battle-mariofox-wait-tick-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_tick -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox Wait tick harness build did not produce the expected ROM and ELF.'
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
        'printf "FTR_MASKS=%#x,%#x,%#x\n", gNdsFighterMarioFoxSetupMask, gNdsFighterMarioFoxStructMask, gNdsFighterMarioFoxInitMask',
        'printf "FTR_WAIT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWaitStatusResult, gNdsFighterMarioFoxWaitMotionResult, gNdsFighterMarioFoxWaitDeferResult, gNdsFighterMarioFoxWaitMask, gNdsFighterMarioFoxWaitDeferredMask, gNdsFighterMarioFoxWaitCount',
        'printf "FTR_TICK=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWaitTickResult, gNdsFighterMarioFoxWaitCallbackResult, gNdsFighterMarioFoxWaitSafeResult, gNdsFighterMarioFoxWaitTickMask, gNdsFighterMarioFoxWaitTickDeferredMask, gNdsFighterMarioFoxWaitTickCount',
        'printf "FTR_TICK_P0=%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterWaitTickP0StatusBefore, gNdsFighterWaitTickP0StatusAfter, gNdsFighterWaitTickP0MotionBefore, gNdsFighterWaitTickP0MotionAfter, gNdsFighterWaitTickP0GABefore, gNdsFighterWaitTickP0GAAfter, gNdsFighterWaitTickP0VelGroundXBeforeBits, gNdsFighterWaitTickP0VelGroundXAfterBits',
        'printf "FTR_TICK_P1=%u,%u,%u,%u,%u,%u,%#x,%#x\n", gNdsFighterWaitTickP1StatusBefore, gNdsFighterWaitTickP1StatusAfter, gNdsFighterWaitTickP1MotionBefore, gNdsFighterWaitTickP1MotionAfter, gNdsFighterWaitTickP1GABefore, gNdsFighterWaitTickP1GAAfter, gNdsFighterWaitTickP1VelGroundXBeforeBits, gNdsFighterWaitTickP1VelGroundXAfterBits',
        'printf "FTR_TICK_ROOT=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", gNdsFighterWaitTickP0RootXBeforeBits, gNdsFighterWaitTickP0RootXAfterBits, gNdsFighterWaitTickP0RootYBeforeBits, gNdsFighterWaitTickP0RootYAfterBits, gNdsFighterWaitTickP1RootXBeforeBits, gNdsFighterWaitTickP1RootXAfterBits, gNdsFighterWaitTickP1RootYBeforeBits, gNdsFighterWaitTickP1RootYAfterBits',
        'printf "FTR_TICK_CALLS=%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitTickOriginalInterruptCount, gNdsFighterWaitTickGroundInterruptCheckCount, gNdsFighterWaitTickPhysicsCallbackCount, gNdsFighterWaitTickMapCallbackCount, gNdsFighterWaitProcInterruptCallCount, gNdsFighterWaitProcPhysicsCallCount, gNdsFighterWaitProcMapCallCount',
        'printf "FTR_TICK_SAFE=%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitTickStatusChangeCount, gNdsFighterWaitTickMotionChangeCount, gNdsFighterWaitTickGADriftCount, gNdsFighterWaitTickRootDriftCount, gNdsFighterWaitTickGObjDelta, gNdsFighterWaitTickDeniedStatusCount, gNdsFighterWaitTickProcessAttachCount, gNdsFighterWaitTickDisplayProbeCount, gNdsFighterWaitTickGameplayUpdateCount',
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
    $masks = [regex]::Match($gdbStdout, 'FTR_MASKS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $wait = [regex]::Match($gdbStdout, 'FTR_WAIT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $tick = [regex]::Match($gdbStdout, 'FTR_TICK=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $p0 = [regex]::Match($gdbStdout, 'FTR_TICK_P0=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $p1 = [regex]::Match($gdbStdout, 'FTR_TICK_P1=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $rootTick = [regex]::Match($gdbStdout, 'FTR_TICK_ROOT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $calls = [regex]::Match($gdbStdout, 'FTR_TICK_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $safe = [regex]::Match($gdbStdout, 'FTR_TICK_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    if (-not $harn.Success -or (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or [int]$harn.Groups[2].Value -ne 19 -or [int]$harn.Groups[3].Value -ne 22 -or [int]$harn.Groups[4].Value -ne 21 -or (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Battle Mario/Fox Wait tick harness did not select direct VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21 -or [int]$scene.Groups[3].Value -ne 6) {
        throw "Live scene state is not Pupupu VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $vsb.Success -or (Convert-MarkerUInt32 $vsb.Groups[1].Value) -ne 0x56425355 -or ((Convert-MarkerUInt32 $vsb.Groups[2].Value) -band 0x6f) -ne 0x6f -or (Convert-MarkerUInt32 $vsb.Groups[3].Value) -ne 0x56425550 -or [int]$vsb.Groups[4].Value -ne 2 -or [int]$vsb.Groups[5].Value -ne 2 -or [int]$vsb.Groups[6].Value -ne 6) {
        throw "Imported VSBattle setup did not reach the Wait tick boundary.`n$gdbStdout"
    }
    if (-not $stage.Success -or (Convert-MarkerUInt32 $stage.Groups[1].Value) -ne 0x50555042 -or ((Convert-MarkerUInt32 $stage.Groups[2].Value) -band 0xff) -ne 0xff -or (Convert-MarkerUInt32 $stage.Groups[3].Value) -ne 0x50554753 -or ((Convert-MarkerUInt32 $stage.Groups[4].Value) -band 0x3ff) -ne 0x3ff -or [int]$stage.Groups[5].Value -ne 4) {
        throw "Pupupu stage/ground setup failed before Wait tick.`n$gdbStdout"
    }
    if (-not $base.Success -or (Convert-MarkerUInt32 $base.Groups[1].Value) -ne 0x46544d44 -or (Convert-MarkerUInt32 $base.Groups[2].Value) -ne 0x4654474f -or (Convert-MarkerUInt32 $base.Groups[3].Value) -ne 0x46545348 -or (Convert-MarkerUInt32 $base.Groups[4].Value) -ne 0x46544a54 -or (Convert-MarkerUInt32 $base.Groups[5].Value) -ne 0x46545354 -or (Convert-MarkerUInt32 $base.Groups[6].Value) -ne 0x4654494e -or (Convert-MarkerUInt32 $base.Groups[7].Value) -ne 0x4654434c -or [int]$base.Groups[8].Value -ne 2 -or [int]$base.Groups[9].Value -ne 2 -or [int]$base.Groups[10].Value -ne 2) {
        throw "Mario/Fox model/struct/init base proof failed before Wait tick.`n$gdbStdout"
    }
    if (-not $masks.Success -or ((Convert-MarkerUInt32 $masks.Groups[1].Value) -band 0xfff) -ne 0xfff -or ((Convert-MarkerUInt32 $masks.Groups[3].Value) -band 0x3fff) -ne 0x3fff) {
        throw "Mario/Fox base proof masks are incomplete before Wait tick.`n$gdbStdout"
    }
    if (-not $wait.Success -or (Convert-MarkerUInt32 $wait.Groups[1].Value) -ne 0x46545753 -or (Convert-MarkerUInt32 $wait.Groups[2].Value) -ne 0x4654574d -or (Convert-MarkerUInt32 $wait.Groups[3].Value) -ne 0x46545744 -or ((Convert-MarkerUInt32 $wait.Groups[4].Value) -band 0xfff) -ne 0xfff -or (Convert-MarkerUInt32 $wait.Groups[5].Value) -ne 0xff -or [int]$wait.Groups[6].Value -ne 2) {
        throw "Mario/Fox Wait setup proof failed before tick.`n$gdbStdout"
    }
    if (-not $tick.Success -or (Convert-MarkerUInt32 $tick.Groups[1].Value) -ne 0x4654544b -or (Convert-MarkerUInt32 $tick.Groups[2].Value) -ne 0x46544342 -or (Convert-MarkerUInt32 $tick.Groups[3].Value) -ne 0x46545346 -or ((Convert-MarkerUInt32 $tick.Groups[4].Value) -band 0x3ff) -ne 0x3ff -or (Convert-MarkerUInt32 $tick.Groups[5].Value) -ne 0xff -or [int]$tick.Groups[6].Value -ne 2) {
        throw "Mario/Fox Wait tick proof failed.`n$gdbStdout"
    }
    foreach ($player in @($p0, $p1)) {
        if (-not $player.Success -or [int]$player.Groups[1].Value -ne 10 -or [int]$player.Groups[2].Value -ne 10 -or [int]$player.Groups[3].Value -ne 4 -or [int]$player.Groups[4].Value -ne 4 -or [int]$player.Groups[5].Value -ne 0 -or [int]$player.Groups[6].Value -ne 0 -or (Convert-MarkerUInt32 $player.Groups[7].Value) -ne (Convert-MarkerUInt32 $player.Groups[8].Value)) {
            throw "One Mario/Fox Wait tick state record is incorrect.`n$gdbStdout"
        }
    }
    if (-not $rootTick.Success -or (Convert-MarkerUInt32 $rootTick.Groups[1].Value) -ne (Convert-MarkerUInt32 $rootTick.Groups[2].Value) -or (Convert-MarkerUInt32 $rootTick.Groups[3].Value) -ne (Convert-MarkerUInt32 $rootTick.Groups[4].Value) -or (Convert-MarkerUInt32 $rootTick.Groups[5].Value) -ne (Convert-MarkerUInt32 $rootTick.Groups[6].Value) -or (Convert-MarkerUInt32 $rootTick.Groups[7].Value) -ne (Convert-MarkerUInt32 $rootTick.Groups[8].Value)) {
        throw "Root DObj position drifted during Wait tick.`n$gdbStdout"
    }
    if (-not $calls.Success -or [int]$calls.Groups[1].Value -ne 2 -or [int]$calls.Groups[2].Value -ne 2 -or [int]$calls.Groups[3].Value -ne 2 -or [int]$calls.Groups[4].Value -ne 2 -or [int]$calls.Groups[5].Value -ne 2 -or [int]$calls.Groups[6].Value -ne 2 -or [int]$calls.Groups[7].Value -ne 2) {
        throw "Wait tick callback counters failed.`n$gdbStdout"
    }
    if (-not $safe.Success -or [int]$safe.Groups[1].Value -ne 0 -or [int]$safe.Groups[2].Value -ne 0 -or [int]$safe.Groups[3].Value -ne 0 -or [int]$safe.Groups[4].Value -ne 0 -or [int]$safe.Groups[5].Value -ne 0 -or [int]$safe.Groups[6].Value -ne 0 -or [int]$safe.Groups[7].Value -ne 0 -or [int]$safe.Groups[8].Value -ne 0 -or [int]$safe.Groups[9].Value -ne 0) {
        throw "Wait tick escaped the bounded safety contract.`n$gdbStdout"
    }
    if (-not $boundary.Success -or (Convert-MarkerUInt32 $boundary.Groups[1].Value) -ne 0x53434e45 -or [int]$boundary.Groups[2].Value -ne 22) {
        throw "VSBattle did not park at the bounded scene boundary.`n$gdbStdout"
    }
    Write-Output ("Battle Mario/Fox Wait tick harness passed: scene=22/21 tick={0} callbacks=2/2/2 stable=1 final=22" -f $tick.Groups[4].Value)
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
