param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [switch]$ImportBattleShipFTManager,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ImportBattleShipFTManager = $true
$verifierContext = Initialize-MelonDSVerifierContext `
    -Root $root `
    -MelonDS $MelonDS `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$target = 'smash64ds-battle-mariofox-wait'
$build = 'build-battle-mariofox-wait-harness'
$rom = Join-Path $root ("{0}.nds" -f $target)
$elf = Join-Path $root ("{0}.elf" -f $target)
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-mariofox-wait-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-mariofox-wait-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_mariofox_wait_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
$makeArgs = @('-C', $root, "TARGET=$target", "BUILD=$build", 'NDS_DEV_SCENE_HARNESS=battle_mariofox_wait', '-j16')
if ($ImportBattleShipFTManager) {
    $makeArgs += 'NDS_IMPORT_BATTLESHIP_FTMANAGER=1'
}
& make @makeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle Mario/Fox Wait harness build did not produce the expected ROM and ELF.'
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
        'printf "FTR_MANAGER=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterManagerResult, gNdsFighterManagerMask, gNdsFighterManagerExternMask, gNdsFighterManagerStatusBufferMask, gNdsFighterManagerFighterMask, gNdsFighterManagerDataMask, gNdsFighterManagerWaitMask, gNdsFighterManagerEntryMask, gNdsFighterManagerStatusBufferHitCount, gNdsFighterManagerFighterCount, gNdsFighterManagerFigatreeHeapSize',
        'printf "FTR_BASE=%#x,%#x,%#x,%#x,%#x,%#x,%#x,%u,%u,%u\n", gNdsFighterMarioFoxModelResult, gNdsFighterMarioFoxGObjResult, gNdsFighterMarioFoxStructResult, gNdsFighterMarioFoxJointResult, gNdsFighterMarioFoxStateResult, gNdsFighterMarioFoxInitResult, gNdsFighterMarioFoxCollResult, gNdsFighterModelRealGObjCount, gNdsFighterMarioFoxStructCount, gNdsFighterMarioFoxInitCount',
        'printf "FTR_MASKS=%#x,%#x,%#x\n", gNdsFighterMarioFoxSetupMask, gNdsFighterMarioFoxStructMask, gNdsFighterMarioFoxInitMask',
        'printf "FTR_WAIT=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsFighterMarioFoxWaitStatusResult, gNdsFighterMarioFoxWaitMotionResult, gNdsFighterMarioFoxWaitDeferResult, gNdsFighterMarioFoxWaitMask, gNdsFighterMarioFoxWaitDeferredMask, gNdsFighterMarioFoxWaitCount',
        'printf "FTR_WAIT_P0=%#x,%u,%u,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitP0StatusPrev, gNdsFighterWaitP0StatusID, gNdsFighterWaitP0MotionID, gNdsFighterWaitP0MotionAttackID, gNdsFighterWaitP0StatusAttackID, gNdsFighterWaitP0AnimFrameBits, gNdsFighterWaitP0AnimSpeedBits, gNdsFighterWaitP0SpecialInterrupt, gNdsFighterWaitP0PlayerTagWait, gNdsFighterWaitP0ProcInterruptReady, gNdsFighterWaitP0ProcPhysicsReady, gNdsFighterWaitP0ProcMapReady, gNdsFighterWaitP0MainMotionReady',
        'printf "FTR_WAIT_P1=%#x,%u,%u,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitP1StatusPrev, gNdsFighterWaitP1StatusID, gNdsFighterWaitP1MotionID, gNdsFighterWaitP1MotionAttackID, gNdsFighterWaitP1StatusAttackID, gNdsFighterWaitP1AnimFrameBits, gNdsFighterWaitP1AnimSpeedBits, gNdsFighterWaitP1SpecialInterrupt, gNdsFighterWaitP1PlayerTagWait, gNdsFighterWaitP1ProcInterruptReady, gNdsFighterWaitP1ProcPhysicsReady, gNdsFighterWaitP1ProcMapReady, gNdsFighterWaitP1MainMotionReady',
        'printf "FTR_WAIT_CALLS=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsFighterWaitOriginalSetStatusCallCount, gNdsFighterWaitFtMainSetStatusCallCount, gNdsFighterWaitHammerCheckCount, gNdsFighterWaitHammerDeniedCount, gNdsFighterWaitGroundSetCount, gNdsFighterWaitPlayerTagSetCount, gNdsFighterWaitProcInterruptCallCount, gNdsFighterWaitProcPhysicsCallCount, gNdsFighterWaitProcMapCallCount, gNdsFighterWaitProcessAttachCount, gNdsFighterWaitDisplayProbeCount, gNdsFighterWaitGameplayUpdateCount',
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+),([0-9]+)')
    $vsb = [regex]::Match($gdbStdout, 'VSB=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $manager = [regex]::Match($gdbStdout, 'FTR_MANAGER=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $base = [regex]::Match($gdbStdout, 'FTR_BASE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $masks = [regex]::Match($gdbStdout, 'FTR_MASKS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $wait = [regex]::Match($gdbStdout, 'FTR_WAIT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $p0 = [regex]::Match($gdbStdout, 'FTR_WAIT_P0=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $p1 = [regex]::Match($gdbStdout, 'FTR_WAIT_P1=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $calls = [regex]::Match($gdbStdout, 'FTR_WAIT_CALLS=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    if (-not $harn.Success -or (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or [int]$harn.Groups[2].Value -ne 17 -or [int]$harn.Groups[3].Value -ne 22 -or [int]$harn.Groups[4].Value -ne 21 -or (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Battle Mario/Fox Wait harness did not select direct VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21 -or [int]$scene.Groups[3].Value -ne 6) {
        throw "Live scene state is not Pupupu VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $vsb.Success -or (Convert-MarkerUInt32 $vsb.Groups[1].Value) -ne 0x56425355 -or ((Convert-MarkerUInt32 $vsb.Groups[2].Value) -band 0x7f) -ne 0x7f -or (Convert-MarkerUInt32 $vsb.Groups[3].Value) -ne 0x56425550 -or [int]$vsb.Groups[4].Value -ne 2 -or [int]$vsb.Groups[5].Value -ne 2 -or [int]$vsb.Groups[6].Value -ne 6) {
        throw "Imported VSBattle setup did not reach the Wait boundary.`n$gdbStdout"
    }
    if ($ImportBattleShipFTManager) {
        if (-not $manager.Success -or
            (Convert-MarkerUInt32 $manager.Groups[1].Value) -ne 0x46544d47 -or
            ((Convert-MarkerUInt32 $manager.Groups[2].Value) -band 0xff) -ne 0xff -or
            ((Convert-MarkerUInt32 $manager.Groups[5].Value) -band 0x3) -ne 0x3 -or
            ((Convert-MarkerUInt32 $manager.Groups[6].Value) -band 0x3) -ne 0x3 -or
            [int]$manager.Groups[9].Value -lt 13 -or
            [int]$manager.Groups[10].Value -ne 2 -or
            [int]$manager.Groups[11].Value -eq 0) {
            throw "BattleShip ftmanager Wait harness proof failed.`n$gdbStdout"
        }
        Write-Output ("Battle Mario/Fox Wait ftmanager harness passed: mask={0} wait={1} entry={2} fighters={3}" -f $manager.Groups[2].Value, $manager.Groups[7].Value, $manager.Groups[8].Value, $manager.Groups[10].Value)
        return
    }
    if (-not $base.Success -or (Convert-MarkerUInt32 $base.Groups[1].Value) -ne 0x46544d44 -or (Convert-MarkerUInt32 $base.Groups[2].Value) -ne 0x4654474f -or (Convert-MarkerUInt32 $base.Groups[3].Value) -ne 0x46545348 -or (Convert-MarkerUInt32 $base.Groups[4].Value) -ne 0x46544a54 -or (Convert-MarkerUInt32 $base.Groups[5].Value) -ne 0x46545354 -or (Convert-MarkerUInt32 $base.Groups[6].Value) -ne 0x4654494e -or (Convert-MarkerUInt32 $base.Groups[7].Value) -ne 0x4654434c -or [int]$base.Groups[8].Value -ne 2 -or [int]$base.Groups[9].Value -ne 2 -or [int]$base.Groups[10].Value -ne 2) {
        throw "Mario/Fox model/struct/init base proof failed before Wait.`n$gdbStdout"
    }
    if (-not $masks.Success -or ((Convert-MarkerUInt32 $masks.Groups[1].Value) -band 0xfff) -ne 0xfff -or ((Convert-MarkerUInt32 $masks.Groups[3].Value) -band 0x3fff) -ne 0x3fff) {
        throw "Mario/Fox base proof masks are incomplete before Wait.`n$gdbStdout"
    }
    if (-not $wait.Success -or (Convert-MarkerUInt32 $wait.Groups[1].Value) -ne 0x46545753 -or (Convert-MarkerUInt32 $wait.Groups[2].Value) -ne 0x4654574d -or (Convert-MarkerUInt32 $wait.Groups[3].Value) -ne 0x46545744 -or ((Convert-MarkerUInt32 $wait.Groups[4].Value) -band 0xfff) -ne 0xfff -or (Convert-MarkerUInt32 $wait.Groups[5].Value) -ne 0xff -or [int]$wait.Groups[6].Value -ne 2) {
        throw "Mario/Fox Wait proof failed.`n$gdbStdout"
    }
    foreach ($player in @($p0, $p1)) {
        if (-not $player.Success -or
            (Convert-MarkerUInt32 $player.Groups[1].Value) -ne [uint32]::MaxValue -or
            [int]$player.Groups[2].Value -ne 10 -or
            [int]$player.Groups[3].Value -ne 4 -or
            (Convert-MarkerUInt32 $player.Groups[4].Value) -ne 0 -or
            (Convert-MarkerUInt32 $player.Groups[5].Value) -ne 0 -or
            (Convert-MarkerUInt32 $player.Groups[6].Value) -ne 0 -or
            (Convert-MarkerUInt32 $player.Groups[7].Value) -ne 0x3f800000 -or
            [int]$player.Groups[8].Value -ne 1 -or
            [int]$player.Groups[9].Value -ne 120 -or
            [int]$player.Groups[10].Value -ne 1 -or
            [int]$player.Groups[11].Value -ne 1 -or
            [int]$player.Groups[12].Value -ne 1 -or
            [int]$player.Groups[13].Value -ne 1) {
            throw "One Mario/Fox Wait status record is incorrect.`n$gdbStdout"
        }
    }
    if (-not $calls.Success -or [int]$calls.Groups[1].Value -ne 2 -or [int]$calls.Groups[2].Value -ne 2 -or [int]$calls.Groups[3].Value -ne 2 -or [int]$calls.Groups[4].Value -ne 0 -or [int]$calls.Groups[6].Value -ne 2 -or [int]$calls.Groups[7].Value -ne 0 -or [int]$calls.Groups[8].Value -ne 0 -or [int]$calls.Groups[9].Value -ne 0 -or [int]$calls.Groups[10].Value -ne 0 -or [int]$calls.Groups[11].Value -ne 0 -or [int]$calls.Groups[12].Value -ne 0) {
        throw "Wait compatibility/deferred call counters failed.`n$gdbStdout"
    }
    if (-not $boundary.Success -or (Convert-MarkerUInt32 $boundary.Groups[1].Value) -ne 0x53434e45 -or [int]$boundary.Groups[2].Value -ne 22) {
        throw "VSBattle did not park at the bounded scene boundary.`n$gdbStdout"
    }
    Write-Output ("Battle Mario/Fox Wait harness passed: waitMask={0} count={1} status=10/10 motion=4/4 callbacks=0" -f $wait.Groups[4].Value, $wait.Groups[6].Value)
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
