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
$rom = Join-Path $root 'smash64ds-menu-chain-mariofox-wait.nds'
$elf = Join-Path $root 'smash64ds-menu-chain-mariofox-wait.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.menu-chain-mariofox-wait-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.menu-chain-mariofox-wait-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_menu_chain_mariofox_wait_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-menu-chain-mariofox-wait BUILD=build-menu-chain-mariofox-wait-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Menu-chain Mario/Fox Wait harness build did not produce the expected ROM and ELF.'
}
if (-not (Test-Path $melonDsPath)) { throw "melonDS executable not found: $melonDsPath" }
if (-not (Test-Path $Gdb)) { throw "GDB executable not found: $Gdb" }
function Assert-WaitPlayer {
    param([System.Text.RegularExpressions.Match]$Match, [string]$Name, [string]$GdbText)
    if (-not $Match.Success -or
        (Convert-MarkerUInt32 $Match.Groups[1].Value) -ne [uint32]::MaxValue -or
        [int]$Match.Groups[2].Value -ne 10 -or
        [int]$Match.Groups[3].Value -ne 4 -or
        (Convert-MarkerUInt32 $Match.Groups[4].Value) -ne 0 -or
        (Convert-MarkerUInt32 $Match.Groups[5].Value) -ne 0 -or
        (Convert-MarkerUInt32 $Match.Groups[6].Value) -ne 0 -or
        (Convert-MarkerUInt32 $Match.Groups[7].Value) -ne 0x3f800000 -or
        [int]$Match.Groups[8].Value -ne 1 -or
        [int]$Match.Groups[9].Value -ne 120 -or
        [int]$Match.Groups[10].Value -ne 1 -or
        [int]$Match.Groups[11].Value -ne 1 -or
        [int]$Match.Groups[12].Value -ne 1 -or
        [int]$Match.Groups[13].Value -ne 1) {
        throw "$Name Wait status record is incorrect.`n$GdbText"
    }
}
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
        'printf "FTR_BASE=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxModelResult, gNdsFighterMarioFoxGObjResult, gNdsFighterMarioFoxStructResult, gNdsFighterMarioFoxJointResult, gNdsFighterMarioFoxStateResult, gNdsFighterMarioFoxInitResult, gNdsFighterMarioFoxCollResult, gNdsFighterModelRealGObjCount, gNdsFighterMarioFoxStructCount, gNdsFighterMarioFoxInitCount',
        'printf "FTR_MASKS=%#x,%#x,%#x\n", gNdsFighterMarioFoxSetupMask, gNdsFighterMarioFoxStructMask, gNdsFighterMarioFoxInitMask',
        'printf "FTR_WAIT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWaitStatusResult, gNdsFighterMarioFoxWaitMotionResult, gNdsFighterMarioFoxWaitDeferResult, gNdsFighterMarioFoxWaitMask, gNdsFighterMarioFoxWaitDeferredMask, gNdsFighterMarioFoxWaitCount',
        'printf "FTR_WAIT_P0=%#x,%u,%u,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitP0StatusPrev, gNdsFighterWaitP0StatusID, gNdsFighterWaitP0MotionID, gNdsFighterWaitP0MotionAttackID, gNdsFighterWaitP0StatusAttackID, gNdsFighterWaitP0AnimFrameBits, gNdsFighterWaitP0AnimSpeedBits, gNdsFighterWaitP0SpecialInterrupt, gNdsFighterWaitP0PlayerTagWait, gNdsFighterWaitP0ProcInterruptReady, gNdsFighterWaitP0ProcPhysicsReady, gNdsFighterWaitP0ProcMapReady, gNdsFighterWaitP0MainMotionReady',
        'printf "FTR_WAIT_P1=%#x,%u,%u,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitP1StatusPrev, gNdsFighterWaitP1StatusID, gNdsFighterWaitP1MotionID, gNdsFighterWaitP1MotionAttackID, gNdsFighterWaitP1StatusAttackID, gNdsFighterWaitP1AnimFrameBits, gNdsFighterWaitP1AnimSpeedBits, gNdsFighterWaitP1SpecialInterrupt, gNdsFighterWaitP1PlayerTagWait, gNdsFighterWaitP1ProcInterruptReady, gNdsFighterWaitP1ProcPhysicsReady, gNdsFighterWaitP1ProcMapReady, gNdsFighterWaitP1MainMotionReady',
        'printf "FTR_WAIT_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitOriginalSetStatusCallCount, gNdsFighterWaitFtMainSetStatusCallCount, gNdsFighterWaitHammerCheckCount, gNdsFighterWaitHammerDeniedCount, gNdsFighterWaitGroundSetCount, gNdsFighterWaitPlayerTagSetCount, gNdsFighterWaitProcInterruptCallCount, gNdsFighterWaitProcPhysicsCallCount, gNdsFighterWaitProcMapCallCount, gNdsFighterWaitProcessAttachCount, gNdsFighterWaitDisplayProbeCount, gNdsFighterWaitGameplayUpdateCount',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $chain = [regex]::Match($gdbStdout, 'CHAIN=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $final = [regex]::Match($gdbStdout, 'CHAIN_FINAL=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $base = [regex]::Match($gdbStdout, 'FTR_BASE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $masks = [regex]::Match($gdbStdout, 'FTR_MASKS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $wait = [regex]::Match($gdbStdout, 'FTR_WAIT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $p0 = [regex]::Match($gdbStdout, 'FTR_WAIT_P0=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $p1 = [regex]::Match($gdbStdout, 'FTR_WAIT_P1=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $calls = [regex]::Match($gdbStdout, 'FTR_WAIT_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    if (-not $harn.Success -or (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or [int]$harn.Groups[2].Value -ne 18 -or [int]$harn.Groups[3].Value -ne 9 -or [int]$harn.Groups[4].Value -ne 1 -or (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Menu-chain Mario/Fox Wait harness did not start at VSMode from Title.`n$gdbStdout"
    }
    if (-not $chain.Success -or (Convert-MarkerUInt32 $chain.Groups[1].Value) -ne 0x56535452 -or ((Convert-MarkerUInt32 $chain.Groups[2].Value) -band 0xff) -ne 0xff -or (Convert-MarkerUInt32 $chain.Groups[3].Value) -ne 0x50565452 -or ((Convert-MarkerUInt32 $chain.Groups[4].Value) -band 0xff) -ne 0xff -or (Convert-MarkerUInt32 $chain.Groups[5].Value) -ne 0x4d53454c -or ((Convert-MarkerUInt32 $chain.Groups[6].Value) -band 0xff) -ne 0xff) {
        throw "Menu-chain transitions did not reach VSBattle.`n$gdbStdout"
    }
    if (-not $final.Success -or [int]$final.Groups[1].Value -ne 16 -or [int]$final.Groups[2].Value -ne 21 -or [int]$final.Groups[3].Value -ne 22 -or [int]$final.Groups[4].Value -ne 21 -or [int]$final.Groups[5].Value -ne 6) {
        throw "Menu-chain final scene state is not Pupupu VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21 -or [int]$scene.Groups[3].Value -ne 6) {
        throw "Live scene state is not VSBattle after menu chain.`n$gdbStdout"
    }
    if (-not $base.Success -or (Convert-MarkerUInt32 $base.Groups[1].Value) -ne 0x46544d44 -or (Convert-MarkerUInt32 $base.Groups[2].Value) -ne 0x4654474f -or (Convert-MarkerUInt32 $base.Groups[3].Value) -ne 0x46545348 -or (Convert-MarkerUInt32 $base.Groups[4].Value) -ne 0x46544a54 -or (Convert-MarkerUInt32 $base.Groups[5].Value) -ne 0x46545354 -or (Convert-MarkerUInt32 $base.Groups[6].Value) -ne 0x4654494e -or (Convert-MarkerUInt32 $base.Groups[7].Value) -ne 0x4654434c -or [int]$base.Groups[8].Value -ne 2 -or [int]$base.Groups[9].Value -ne 2 -or [int]$base.Groups[10].Value -ne 2) {
        throw "Mario/Fox model/struct/init base proof failed after menu chain.`n$gdbStdout"
    }
    if (-not $masks.Success -or ((Convert-MarkerUInt32 $masks.Groups[1].Value) -band 0xfff) -ne 0xfff -or ((Convert-MarkerUInt32 $masks.Groups[3].Value) -band 0x3fff) -ne 0x3fff) {
        throw "Mario/Fox base proof masks are incomplete after menu chain.`n$gdbStdout"
    }
    if (-not $wait.Success -or (Convert-MarkerUInt32 $wait.Groups[1].Value) -ne 0x46545753 -or (Convert-MarkerUInt32 $wait.Groups[2].Value) -ne 0x4654574d -or (Convert-MarkerUInt32 $wait.Groups[3].Value) -ne 0x46545744 -or ((Convert-MarkerUInt32 $wait.Groups[4].Value) -band 0xfff) -ne 0xfff -or (Convert-MarkerUInt32 $wait.Groups[5].Value) -ne 0xff -or [int]$wait.Groups[6].Value -ne 2) {
        throw "Mario/Fox Wait proof failed after menu chain.`n$gdbStdout"
    }
    Assert-WaitPlayer -Match $p0 -Name 'Mario' -GdbText $gdbStdout
    Assert-WaitPlayer -Match $p1 -Name 'Fox' -GdbText $gdbStdout
    if (-not $calls.Success -or [int]$calls.Groups[1].Value -ne 2 -or [int]$calls.Groups[2].Value -ne 2 -or [int]$calls.Groups[3].Value -ne 2 -or [int]$calls.Groups[4].Value -ne 0 -or [int]$calls.Groups[6].Value -ne 2 -or [int]$calls.Groups[7].Value -ne 0 -or [int]$calls.Groups[8].Value -ne 0 -or [int]$calls.Groups[9].Value -ne 0 -or [int]$calls.Groups[10].Value -ne 0 -or [int]$calls.Groups[11].Value -ne 0 -or [int]$calls.Groups[12].Value -ne 0) {
        throw "Wait compatibility/deferred call counters failed after menu chain.`n$gdbStdout"
    }
    Write-Output ("Menu-chain Mario/Fox Wait harness passed: chain final=22/21 waitMask={0} count={1} status=10/10 motion=4/4 callbacks=0" -f $wait.Groups[4].Value, $wait.Groups[6].Value)
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
