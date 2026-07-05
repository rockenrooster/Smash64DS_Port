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
$rom = Join-Path $root 'smash64ds-battle-fd.nds'
$elf = Join-Path $root 'smash64ds-battle-fd.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.battle-fd-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.battle-fd-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_battle_fd_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-battle-fd BUILD=build-battle-fd-harness NDS_DEV_SCENE_HARNESS=battle_fd -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Battle FD harness build did not produce smash64ds-battle-fd.nds and smash64ds-battle-fd.elf.'
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
        'printf "SCENE=%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev',
        'printf "VSB_SETUP=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%u\n", gNdsSCVSBattleOriginalStartResult, gNdsSCVSBattleOriginalFuncStartResult, gNdsSCVSBattleOriginalRelocResult, gNdsSCVSBattleOriginalSetupResult, gNdsSCVSBattleOriginalSetupMask, gNdsSCVSBattleOriginalLoadedFileCount, gNdsSCVSBattleOriginalGObjCount, gNdsSCVSBattleOriginalCameraCount, gNdsSCVSBattleOriginalFighterGObjCount, gNdsSCVSBattleOriginalActivePlayerMask, gNdsSCVSBattleCompatMask, gNdsSCVSBattleOriginalPlayerCount, gNdsSCVSBattleOriginalCpuCount, gNdsSCVSBattleOriginalGameRule, gNdsSCVSBattleOriginalStock, gNdsSCVSBattleOriginalGKind, gNdsSCVSBattleOriginalSceneCurr, gNdsSCVSBattleOriginalScenePrev',
        'printf "VSB_COMPAT=%#x,%#x,%#x,%#x,%#x,%#x,%u,%u\n", gNdsSCVSBattleCompatCameraMask, gNdsSCVSBattleCompatInterfaceMask, gNdsSCVSBattleCompatManagerMask, gNdsSCVSBattleCompatAudioMask, gNdsSCVSBattleCompatSpawnMask, gNdsSCVSBattleCompatMask, gNdsSCVSBattleLastAudioVolume, gNdsSCVSBattleLastFGM',
        'printf "VSB_UPDATE=%#x,%u\n", gNdsSCVSBattleOriginalUpdateResult, gNdsSCVSBattleOriginalUpdateCount',
        'printf "TASK=%u,%u,%u,%u,%u,%u\n", gNdsTaskmanLoopReached, gNdsTaskmanSceneUpdateSet, gNdsTaskmanSceneDrawSet, gNdsTaskmanControllerAutoRead, gNdsSceneBoundaryKind, gNdsTaskmanBoundedUpdateCount',
        'printf "ROOM_TITLE=%u,%#x,%#x\n", gNdsOpeningRoomTickCount, gNdsTitleOriginalStartResult, gNdsTitleDrawResult',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+)')
    $setup = [regex]::Match($gdbStdout, 'VSB_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $compat = [regex]::Match($gdbStdout, 'VSB_COMPAT=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+)')
    $update = [regex]::Match($gdbStdout, 'VSB_UPDATE=(0x[0-9a-fA-F]+|0),([0-9]+)')
    $task = [regex]::Match($gdbStdout, 'TASK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $roomTitle = [regex]::Match($gdbStdout, 'ROOM_TITLE=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 3 -or
        [int]$harn.Groups[3].Value -ne 22 -or
        [int]$harn.Groups[4].Value -ne 21 -or
        (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "Battle FD harness did not select direct VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21) {
        throw "Live scene state is not VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $setup.Success -or
        (Convert-MarkerUInt32 $setup.Groups[1].Value) -ne 0x56425354 -or
        (Convert-MarkerUInt32 $setup.Groups[2].Value) -ne 0x56424653 -or
        (Convert-MarkerUInt32 $setup.Groups[3].Value) -ne 0x5642524c -or
        (Convert-MarkerUInt32 $setup.Groups[4].Value) -ne 0x56425355 -or
        ((Convert-MarkerUInt32 $setup.Groups[5].Value) -band 0x6f) -ne 0x6f -or
        [int]$setup.Groups[6].Value -ne 8 -or
        [int]$setup.Groups[8].Value -lt 1 -or
        [int]$setup.Groups[9].Value -ne 1 -or
        [int]$setup.Groups[10].Value -ne 1 -or
        ((Convert-MarkerUInt32 $setup.Groups[11].Value) -band 0xff) -ne 0xff -or
        [int]$setup.Groups[12].Value -ne 1 -or
        [int]$setup.Groups[13].Value -ne 0 -or
        [int]$setup.Groups[14].Value -ne 2 -or
        [int]$setup.Groups[15].Value -ne 3 -or
        [int]$setup.Groups[16].Value -ne 16 -or
        [int]$setup.Groups[17].Value -ne 22 -or
        [int]$setup.Groups[18].Value -ne 21) {
        throw "Imported VSBattle setup proof failed.`n$gdbStdout"
    }
    if (-not $compat.Success -or
        ((Convert-MarkerUInt32 $compat.Groups[1].Value) -band 0x3f) -ne 0x3f -or
        ((Convert-MarkerUInt32 $compat.Groups[2].Value) -band 0x37ff) -ne 0x37ff -or
        ((Convert-MarkerUInt32 $compat.Groups[3].Value) -band 0x1ff) -ne 0x1ff -or
        ((Convert-MarkerUInt32 $compat.Groups[4].Value) -band 0x12) -ne 0x12 -or
        ((Convert-MarkerUInt32 $compat.Groups[5].Value) -band 0x1) -ne 0x1 -or
        ((Convert-MarkerUInt32 $compat.Groups[6].Value) -band 0xff) -ne 0xff -or
        [int]$compat.Groups[8].Value -ne 626) {
        throw "VSBattle compatibility stub coverage failed.`n$gdbStdout"
    }
    if (-not $update.Success -or (Convert-MarkerUInt32 $update.Groups[1].Value) -ne 0x56425550 -or [int]$update.Groups[2].Value -ne 1) {
        throw "VSBattle interface update tick was not proven.`n$gdbStdout"
    }
    if (-not $task.Success -or [int]$task.Groups[1].Value -ne 1 -or [int]$task.Groups[5].Value -ne 22 -or [int]$task.Groups[6].Value -lt 1) {
        throw "Taskman did not park at the VSBattle setup boundary.`n$gdbStdout"
    }
    if (-not $roomTitle.Success -or [int]$roomTitle.Groups[1].Value -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[2].Value) -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[3].Value) -ne 0) {
        throw "Battle FD harness replayed opening/title work.`n$gdbStdout"
    }
    Write-Output ("Battle FD harness passed: files={0} players={1}/{2} fighters={3} gkind={4} mask={5}" -f $setup.Groups[6].Value, $setup.Groups[12].Value, $setup.Groups[13].Value, $setup.Groups[9].Value, $setup.Groups[16].Value, $setup.Groups[5].Value)
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
