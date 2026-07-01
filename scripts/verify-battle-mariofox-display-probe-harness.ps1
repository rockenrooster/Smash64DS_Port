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
$rom = Join-Path $root 'smash64ds-battle-mariofox-display-probe.nds'
$elf = Join-Path $root 'smash64ds-battle-mariofox-display-probe.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-display-probe-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-display-probe-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_display_probe_harness.gdb'
function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context)
    if (-not $Condition) { throw "$Message`n$Context" }
}
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-mariofox-display-probe BUILD=build-battle-mariofox-display-probe-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_display_probe -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox display probe harness build did not produce the expected ROM and ELF.'
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
        'printf "FTR_BASE=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxModelResult, gNdsFighterMarioFoxGObjResult, gNdsFighterMarioFoxStructResult, gNdsFighterMarioFoxJointResult, gNdsFighterMarioFoxStateResult, gNdsFighterMarioFoxInitResult, gNdsFighterMarioFoxCollResult, gNdsFighterModelRealGObjCount, gNdsFighterMarioFoxStructCount, gNdsFighterMarioFoxInitCount',
        'printf "FTR_WAIT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWaitStatusResult, gNdsFighterMarioFoxWaitMotionResult, gNdsFighterMarioFoxWaitDeferResult, gNdsFighterMarioFoxWaitMask, gNdsFighterMarioFoxWaitDeferredMask, gNdsFighterMarioFoxWaitCount',
        'printf "FTR_TICK=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWaitTickResult, gNdsFighterMarioFoxWaitCallbackResult, gNdsFighterMarioFoxWaitSafeResult, gNdsFighterMarioFoxWaitTickMask, gNdsFighterMarioFoxWaitTickDeferredMask, gNdsFighterMarioFoxWaitTickCount',
        'printf "FTR_GROUND=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxGroundPhysResult, gNdsFighterMarioFoxGroundMapResult, gNdsFighterMarioFoxGroundSafeResult, gNdsFighterMarioFoxGroundMask, gNdsFighterMarioFoxGroundDeferredMask, gNdsFighterMarioFoxGroundCount',
        'printf "FTR_DISPLAY=%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxDisplayResult, gNdsFighterMarioFoxDisplaySafeResult, gNdsFighterMarioFoxDisplayMask, gNdsFighterMarioFoxDisplayDeferredMask, gNdsFighterMarioFoxDisplayCallbackCount',
        'printf "FTR_DISPLAY_COUNTS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterDisplayP0DObjCount, gNdsFighterDisplayP1DObjCount, gNdsFighterDisplayP0MObjCount, gNdsFighterDisplayP1MObjCount, gNdsFighterDisplayP0AObjCount, gNdsFighterDisplayP1AObjCount, gNdsFighterDisplayP0DLReadyCount, gNdsFighterDisplayP1DLReadyCount, gNdsFighterDisplayP0PartsPtrCount, gNdsFighterDisplayP1PartsPtrCount',
        'printf "FTR_DISPLAY_STATE=%u,%u,%u,%u,%u,%u\n", gNdsFighterDisplayP0StatusAfter, gNdsFighterDisplayP1StatusAfter, gNdsFighterDisplayP0MotionAfter, gNdsFighterDisplayP1MotionAfter, gNdsFighterDisplayP0GAAfter, gNdsFighterDisplayP1GAAfter',
        'printf "FTR_DISPLAY_ROOT=%#x,%#x,%#x,%#x\n", gNdsFighterDisplayP0RootXBeforeBits, gNdsFighterDisplayP0RootXAfterBits, gNdsFighterDisplayP1RootXBeforeBits, gNdsFighterDisplayP1RootXAfterBits',
        'printf "FTR_DISPLAY_SAFE=%u,%u,%u,%u\n", gNdsFighterDisplayGObjDelta, gNdsFighterDisplayDrawCallCount, gNdsFighterDisplayMatrixCallCount, gNdsFighterDisplayGameplayUpdateCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $base = [regex]::Match($gdbStdout, 'FTR_BASE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $wait = [regex]::Match($gdbStdout, 'FTR_WAIT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $tick = [regex]::Match($gdbStdout, 'FTR_TICK=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $ground = [regex]::Match($gdbStdout, 'FTR_GROUND=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $display = [regex]::Match($gdbStdout, 'FTR_DISPLAY=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $counts = [regex]::Match($gdbStdout, 'FTR_DISPLAY_COUNTS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $state = [regex]::Match($gdbStdout, 'FTR_DISPLAY_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $rootLine = [regex]::Match($gdbStdout, 'FTR_DISPLAY_ROOT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $safe = [regex]::Match($gdbStdout, 'FTR_DISPLAY_SAFE=([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    Assert-Condition ($harn.Success -and (Convert-MarkerUInt32 $harn.Groups[1].Value) -eq 0x4841524e -and [int]$harn.Groups[2].Value -eq 23 -and [int]$harn.Groups[3].Value -eq 22 -and [int]$harn.Groups[4].Value -eq 21 -and (Convert-MarkerUInt32 $harn.Groups[5].Value) -eq 0) 'Battle Mario/Fox display probe harness did not select direct VSBattle from Maps.' $gdbStdout
    Assert-Condition ($scene.Success -and [int]$scene.Groups[1].Value -eq 22 -and [int]$scene.Groups[2].Value -eq 21 -and [int]$scene.Groups[3].Value -eq 6) 'Live scene state is not Pupupu VSBattle from Maps.' $gdbStdout
    Assert-Condition ($base.Success -and (Convert-MarkerUInt32 $base.Groups[1].Value) -eq 0x46544d44 -and (Convert-MarkerUInt32 $base.Groups[2].Value) -eq 0x4654474f -and (Convert-MarkerUInt32 $base.Groups[3].Value) -eq 0x46545348 -and (Convert-MarkerUInt32 $base.Groups[4].Value) -eq 0x46544a54 -and (Convert-MarkerUInt32 $base.Groups[5].Value) -eq 0x46545354 -and (Convert-MarkerUInt32 $base.Groups[6].Value) -eq 0x4654494e -and (Convert-MarkerUInt32 $base.Groups[7].Value) -eq 0x4654434c -and [int]$base.Groups[8].Value -eq 2 -and [int]$base.Groups[9].Value -eq 2 -and [int]$base.Groups[10].Value -eq 2) 'Mario/Fox model/struct/init base proof failed before display probe.' $gdbStdout
    Assert-Condition ($wait.Success -and (Convert-MarkerUInt32 $wait.Groups[1].Value) -eq 0x46545753 -and (Convert-MarkerUInt32 $wait.Groups[2].Value) -eq 0x4654574d -and (Convert-MarkerUInt32 $wait.Groups[3].Value) -eq 0x46545744 -and ((Convert-MarkerUInt32 $wait.Groups[4].Value) -band 0xfff) -eq 0xfff -and (Convert-MarkerUInt32 $wait.Groups[5].Value) -eq 0xff -and [int]$wait.Groups[6].Value -eq 2) 'Mario/Fox Wait setup proof failed before display probe.' $gdbStdout
    Assert-Condition ($tick.Success -and (Convert-MarkerUInt32 $tick.Groups[1].Value) -eq 0x4654544b -and (Convert-MarkerUInt32 $tick.Groups[2].Value) -eq 0x46544342 -and (Convert-MarkerUInt32 $tick.Groups[3].Value) -eq 0x46545346 -and ((Convert-MarkerUInt32 $tick.Groups[4].Value) -band 0x3ff) -eq 0x3ff -and (Convert-MarkerUInt32 $tick.Groups[5].Value) -eq 0xff -and [int]$tick.Groups[6].Value -eq 2) 'Mario/Fox Wait tick proof failed before display probe.' $gdbStdout
    Assert-Condition ($ground.Success -and (Convert-MarkerUInt32 $ground.Groups[1].Value) -eq 0x46544750 -and (Convert-MarkerUInt32 $ground.Groups[2].Value) -eq 0x4654474d -and (Convert-MarkerUInt32 $ground.Groups[3].Value) -eq 0x46544753 -and ((Convert-MarkerUInt32 $ground.Groups[4].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $ground.Groups[5].Value) -eq 0xff -and [int]$ground.Groups[6].Value -eq 2) 'Mario/Fox Wait ground proof failed before display probe.' $gdbStdout
    Assert-Condition ($display.Success -and (Convert-MarkerUInt32 $display.Groups[1].Value) -eq 0x46544450 -and (Convert-MarkerUInt32 $display.Groups[2].Value) -eq 0x46544453 -and ((Convert-MarkerUInt32 $display.Groups[3].Value) -band 0x7ff) -eq 0x7ff -and (Convert-MarkerUInt32 $display.Groups[4].Value) -eq 0x3f -and [int]$display.Groups[5].Value -eq 2) 'Mario/Fox display metadata probe did not pass.' $gdbStdout
    Assert-Condition ($counts.Success -and [int]$counts.Groups[1].Value -gt 0 -and [int]$counts.Groups[2].Value -gt 0 -and (([int]$counts.Groups[7].Value -gt 0) -or ([int]$counts.Groups[9].Value -gt 0)) -and (([int]$counts.Groups[8].Value -gt 0) -or ([int]$counts.Groups[10].Value -gt 0))) 'Display metadata counts did not find fighter DObj/display candidates.' $gdbStdout
    Assert-Condition ($state.Success -and [int]$state.Groups[1].Value -eq 10 -and [int]$state.Groups[2].Value -eq 10 -and [int]$state.Groups[3].Value -eq 4 -and [int]$state.Groups[4].Value -eq 4 -and [int]$state.Groups[5].Value -eq 0 -and [int]$state.Groups[6].Value -eq 0) 'Display probe changed fighter status, motion, or GA state.' $gdbStdout
    Assert-Condition ($rootLine.Success -and (Convert-MarkerUInt32 $rootLine.Groups[1].Value) -eq (Convert-MarkerUInt32 $rootLine.Groups[2].Value) -and (Convert-MarkerUInt32 $rootLine.Groups[3].Value) -eq (Convert-MarkerUInt32 $rootLine.Groups[4].Value)) 'Display probe moved fighter root DObjs.' $gdbStdout
    Assert-Condition ($safe.Success -and [int]$safe.Groups[1].Value -eq 0 -and [int]$safe.Groups[2].Value -eq 0 -and [int]$safe.Groups[3].Value -eq 0 -and [int]$safe.Groups[4].Value -eq 0) 'Display probe escaped into draw, matrix, gameplay, or object creation behavior.' $gdbStdout
    Assert-Condition ($boundary.Success -and (Convert-MarkerUInt32 $boundary.Groups[1].Value) -eq 0x53434e45 -and [int]$boundary.Groups[2].Value -eq 22) 'VSBattle did not park at the bounded scene boundary.' $gdbStdout
    Write-Output ("Battle Mario/Fox display probe harness passed: scene=22/21 display={0} dobj={1}/{2} mobj={3}/{4} ready={5}/{6} stable=1" -f $display.Groups[3].Value, $counts.Groups[1].Value, $counts.Groups[2].Value, $counts.Groups[3].Value, $counts.Groups[4].Value, ([int]$counts.Groups[7].Value + [int]$counts.Groups[9].Value), ([int]$counts.Groups[8].Value + [int]$counts.Groups[10].Value))
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
