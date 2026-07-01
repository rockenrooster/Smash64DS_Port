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
$rom = Join-Path $root 'smash64ds-players-vs.nds'
$elf = Join-Path $root 'smash64ds-players-vs.elf'
$melonDsPath = $verifierContext.MelonDSPath
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot (Get-MelonDSActiveRunnerSlot)
$stdout = Join-Path $logDir 'melonds.players-vs-setup-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.players-vs-setup-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_players_vs_setup_harness.gdb'
if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
& make -C $root TARGET=smash64ds-players-vs BUILD=build-players-vs-setup-harness NDS_DEV_SCENE_HARNESS=players_setup -j16
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'PlayersVS setup harness build did not produce smash64ds-players-vs.nds and smash64ds-players-vs.elf.'
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
        'printf "PV_SETUP=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%#x\n", gNdsPlayersVSOriginalStartResult, gNdsPlayersVSOriginalFuncStartResult, gNdsPlayersVSOriginalRelocResult, gNdsPlayersVSOriginalSetupResult, gNdsPlayersVSOriginalSetupMask, gNdsPlayersVSOriginalLoadedFileCount, gNdsPlayersVSOriginalGObjCount, gNdsPlayersVSOriginalCameraCount, gNdsPlayersVSOriginalSObjCount, gNdsPlayersVSOriginalMainGObjID, gNdsPlayersVSOriginalDeferredMask',
        'printf "PV_SLOTS=%#x,%#x,%#x,%u,%u,%u,%u,%u\n", gNdsPlayersVSOriginalControllerOrderMask, gNdsPlayersVSOriginalSlotKindMask, gNdsPlayersVSOriginalSlotSelectedMask, gNdsPlayersVSOriginalCursorCount, gNdsPlayersVSOriginalPuckCount, gNdsPlayersVSOriginalGateCount, gNdsPlayersVSOriginalPortraitCount, gNdsPlayersVSOriginalFigatreeHeapCount',
        'printf "PV_STATE=%u,%u,%u,%u,%u\n", gNdsPlayersVSOriginalTime, gNdsPlayersVSOriginalStock, gNdsPlayersVSOriginalGameRule, gNdsPlayersVSOriginalIsTeam, gNdsPlayersVSOriginalIsStageSelect',
        'printf "PV_TRANS=%#x,%#x\n", gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask',
        'printf "TASK=%u,%u,%u,%u,%u\n", gNdsTaskmanLoopReached, gNdsTaskmanSceneUpdateSet, gNdsTaskmanSceneDrawSet, gNdsTaskmanControllerAutoRead, gNdsSceneBoundaryKind',
        'printf "ROOM_TITLE=%u,%#x,%#x\n", gNdsOpeningRoomTickCount, gNdsTitleOriginalStartResult, gNdsTitleDrawResult',
        'detach',
        'quit'
    )
    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout
    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+)')
    $setup = [regex]::Match($gdbStdout, 'PV_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $slots = [regex]::Match($gdbStdout, 'PV_SLOTS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $state = [regex]::Match($gdbStdout, 'PV_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $trans = [regex]::Match($gdbStdout, 'PV_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $task = [regex]::Match($gdbStdout, 'TASK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $roomTitle = [regex]::Match($gdbStdout, 'ROOM_TITLE=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 5 -or
        [int]$harn.Groups[3].Value -ne 16 -or
        [int]$harn.Groups[4].Value -ne 9) {
        throw "PlayersVS setup harness did not select PlayersVS from VSMode.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 16 -or [int]$scene.Groups[2].Value -ne 9) {
        throw "Live scene state is not PlayersVS from VSMode.`n$gdbStdout"
    }
    if (-not $setup.Success -or
        (Convert-MarkerUInt32 $setup.Groups[1].Value) -ne 0x50565354 -or
        (Convert-MarkerUInt32 $setup.Groups[2].Value) -ne 0x50564653 -or
        (Convert-MarkerUInt32 $setup.Groups[3].Value) -ne 0x5056524c -or
        (Convert-MarkerUInt32 $setup.Groups[4].Value) -ne 0x50565355 -or
        ((Convert-MarkerUInt32 $setup.Groups[5].Value) -band 0xff) -ne 0xff -or
        [int]$setup.Groups[6].Value -ne 7 -or
        [int]$setup.Groups[8].Value -lt 1 -or
        [int]$setup.Groups[9].Value -lt 20) {
        throw "Imported PlayersVS setup proof failed.`n$gdbStdout"
    }
    if (-not $slots.Success -or
        (Convert-MarkerUInt32 $slots.Groups[1].Value) -eq 0 -or
        [int]$slots.Groups[4].Value -lt 1 -or
        [int]$slots.Groups[5].Value -lt 4 -or
        [int]$slots.Groups[6].Value -lt 4 -or
        [int]$slots.Groups[7].Value -lt 12 -or
        [int]$slots.Groups[8].Value -lt 4) {
        throw "PlayersVS object/slot proof is incomplete.`n$gdbStdout"
    }
    if (-not $state.Success -or
        [int]$state.Groups[1].Value -ne 3 -or
        [int]$state.Groups[2].Value -ne 2 -or
        [int]$state.Groups[3].Value -ne 1 -or
        [int]$state.Groups[4].Value -ne 0 -or
        [int]$state.Groups[5].Value -ne 1) {
        throw "PlayersVS seeded state is not the expected VS defaults.`n$gdbStdout"
    }
    if (-not $trans.Success -or (Convert-MarkerUInt32 $trans.Groups[1].Value) -ne 0 -or (Convert-MarkerUInt32 $trans.Groups[2].Value) -ne 0) {
        throw "PlayersVS setup harness unexpectedly ran the ready transition.`n$gdbStdout"
    }
    if (-not $task.Success -or [int]$task.Groups[1].Value -ne 1 -or [int]$task.Groups[5].Value -ne 16) {
        throw "Taskman did not park at the PlayersVS boundary.`n$gdbStdout"
    }
    if (-not $roomTitle.Success -or [int]$roomTitle.Groups[1].Value -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[2].Value) -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[3].Value) -ne 0) {
        throw "PlayersVS setup harness replayed opening/title work.`n$gdbStdout"
    }
    Write-Output ("PlayersVS setup harness passed: files={0} mask={1} sobj={2} slots={3}/{4}/{5}" -f $setup.Groups[6].Value, $setup.Groups[5].Value, $setup.Groups[9].Value, $slots.Groups[4].Value, $slots.Groups[5].Value, $slots.Groups[6].Value)
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
