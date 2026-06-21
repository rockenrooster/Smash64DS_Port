param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$DelaySeconds = 5
)

$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$rom = Join-Path $root 'smash64ds-menu-chain.nds'
$elf = Join-Path $root 'smash64ds-menu-chain.elf'
$melonDsPath = Resolve-MelonDSPath -Root $root -MelonDS $MelonDS
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Join-Path $root 'artifacts\emulator-logs'
$stdout = Join-Path $logDir 'melonds.menu-chain-vsbattle-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.menu-chain-vsbattle-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_menu_chain_vsbattle_harness.gdb'

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }

& make -C $root TARGET=smash64ds-menu-chain BUILD=build-menu-chain-vsbattle-harness NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle -j4
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'Menu chain harness build did not produce smash64ds-menu-chain.nds and smash64ds-menu-chain.elf.'
}
if (-not (Test-Path $melonDsPath)) { throw "melonDS executable not found: $melonDsPath" }
if (-not (Test-Path $Gdb)) { throw "GDB executable not found: $Gdb" }

New-Item -ItemType Directory -Path $logDir -Force | Out-Null

try {
    $configState = Enable-MelonDSGdbConfig -MelonDSPath $melonDsPath
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $melonDsPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDsDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru

    Wait-MelonDSGdbListener -Process $emulator | Out-Null
    Start-Sleep -Seconds ([Math]::Max($DelaySeconds, 1))

    $gdbCommands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 5',
        'target remote 127.0.0.1:3333',
        'printf "HARN=%#x,%u,%u,%u,%#x\n", gNdsSceneHarnessResult, gNdsSceneHarnessMode, gNdsSceneHarnessSceneCurr, gNdsSceneHarnessScenePrev, gNdsSceneHarnessReservedMask',
        'printf "SCENE=%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev',
        'printf "VS_TRANS=%#x,%#x,%u,%u,%u\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask, gNdsVSModeStartTransitionSceneCurrFinal, gNdsVSModeStartTransitionScenePrevFinal, gNdsVSModeStartTransitionCleanupCount',
        'printf "PV_SETUP=%#x,%#x,%#x,%#x,%#x,%u\n", gNdsPlayersVSOriginalStartResult, gNdsPlayersVSOriginalFuncStartResult, gNdsPlayersVSOriginalRelocResult, gNdsPlayersVSOriginalSetupResult, gNdsPlayersVSOriginalSetupMask, gNdsPlayersVSOriginalLoadedFileCount',
        'printf "PV_TRANS=%#x,%#x,%u,%#x,%u,%u,%u,%u,%u,%u\n", gNdsPlayersVSReadyTransitionResult, gNdsPlayersVSReadyTransitionMask, gNdsPlayersVSReadyTransitionUpdateCount, gNdsPlayersVSReadyTransitionInputMask, gNdsPlayersVSReadyTransitionScenePrevBefore, gNdsPlayersVSReadyTransitionSceneCurrBefore, gNdsPlayersVSReadyTransitionScenePrevFinal, gNdsPlayersVSReadyTransitionSceneCurrFinal, gNdsPlayersVSReadyTransitionPlayerCount, gNdsPlayersVSReadyTransitionStageSelect',
        'printf "MAPS_SETUP=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%#x\n", gNdsMapsOriginalStartResult, gNdsMapsOriginalFuncStartResult, gNdsMapsOriginalRelocResult, gNdsMapsOriginalSetupResult, gNdsMapsOriginalSetupMask, gNdsMapsOriginalLoadedFileCount, gNdsMapsOriginalCursorSlot, gNdsMapsOriginalGroundKind, gNdsMapsOriginalPreviewDeferred, gNdsMapsOriginalDeferredMask',
        'printf "MAPS_TRANS=%#x,%#x,%u,%#x,%u,%u,%u,%u,%u\n", gNdsMapsSelectTransitionResult, gNdsMapsSelectTransitionMask, gNdsMapsSelectTransitionUpdateCount, gNdsMapsSelectTransitionInputMask, gNdsMapsSelectTransitionScenePrevBefore, gNdsMapsSelectTransitionSceneCurrBefore, gNdsMapsSelectTransitionScenePrevFinal, gNdsMapsSelectTransitionSceneCurrFinal, gNdsMapsSelectTransitionSelectedGKind',
        'printf "TASK=%u,%u,%u,%u,%u\n", gNdsTaskmanLoopReached, gNdsTaskmanSceneUpdateSet, gNdsTaskmanSceneDrawSet, gNdsTaskmanControllerAutoRead, gNdsSceneBoundaryKind',
        'printf "ROOM_TITLE=%u,%#x,%#x\n", gNdsOpeningRoomTickCount, gNdsTitleOriginalStartResult, gNdsTitleDrawResult',
        'detach',
        'quit'
    )

    $gdbStdout = (Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root -Commands $gdbCommands -ScriptName $scriptName).Stdout

    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+)')
    $vs = [regex]::Match($gdbStdout, 'VS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+)')
    $pvSetup = [regex]::Match($gdbStdout, 'PV_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+)')
    $pvTrans = [regex]::Match($gdbStdout, 'PV_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $mapsSetup = [regex]::Match($gdbStdout, 'MAPS_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $mapsTrans = [regex]::Match($gdbStdout, 'MAPS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $task = [regex]::Match($gdbStdout, 'TASK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $roomTitle = [regex]::Match($gdbStdout, 'ROOM_TITLE=([0-9]+),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')

    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 7 -or
        [int]$harn.Groups[3].Value -ne 9 -or
        [int]$harn.Groups[4].Value -ne 1) {
        throw "Menu-chain harness did not start at VSMode from Title.`n$gdbStdout"
    }
    if (-not $vs.Success -or
        (Convert-MarkerUInt32 $vs.Groups[1].Value) -ne 0x56535452 -or
        ((Convert-MarkerUInt32 $vs.Groups[2].Value) -band 0xff) -ne 0xff -or
        [int]$vs.Groups[3].Value -ne 16 -or
        [int]$vs.Groups[4].Value -ne 9 -or
        [int]$vs.Groups[5].Value -ne 1) {
        throw "VSMode did not transition to PlayersVS through original code.`n$gdbStdout"
    }
    if (-not $pvSetup.Success -or
        (Convert-MarkerUInt32 $pvSetup.Groups[1].Value) -ne 0x50565354 -or
        (Convert-MarkerUInt32 $pvSetup.Groups[2].Value) -ne 0x50564653 -or
        (Convert-MarkerUInt32 $pvSetup.Groups[3].Value) -ne 0x5056524c -or
        (Convert-MarkerUInt32 $pvSetup.Groups[4].Value) -ne 0x50565355 -or
        ((Convert-MarkerUInt32 $pvSetup.Groups[5].Value) -band 0xff) -ne 0xff -or
        [int]$pvSetup.Groups[6].Value -ne 7) {
        throw "PlayersVS setup did not run in the menu-chain harness.`n$gdbStdout"
    }
    if (-not $pvTrans.Success -or
        (Convert-MarkerUInt32 $pvTrans.Groups[1].Value) -ne 0x50565452 -or
        ((Convert-MarkerUInt32 $pvTrans.Groups[2].Value) -band 0xff) -ne 0xff -or
        (Convert-MarkerUInt32 $pvTrans.Groups[4].Value) -ne 0x1000 -or
        [int]$pvTrans.Groups[5].Value -ne 9 -or
        [int]$pvTrans.Groups[6].Value -ne 16 -or
        [int]$pvTrans.Groups[7].Value -ne 16 -or
        [int]$pvTrans.Groups[8].Value -ne 21 -or
        [int]$pvTrans.Groups[9].Value -lt 2 -or
        [int]$pvTrans.Groups[10].Value -ne 1) {
        throw "PlayersVS did not transition to Maps through original ready/start logic.`n$gdbStdout"
    }
    if (-not $mapsSetup.Success -or
        (Convert-MarkerUInt32 $mapsSetup.Groups[1].Value) -ne 0x4d415053 -or
        (Convert-MarkerUInt32 $mapsSetup.Groups[2].Value) -ne 0x4d504653 -or
        (Convert-MarkerUInt32 $mapsSetup.Groups[3].Value) -ne 0x4d50524c -or
        (Convert-MarkerUInt32 $mapsSetup.Groups[4].Value) -ne 0x4d505355 -or
        ((Convert-MarkerUInt32 $mapsSetup.Groups[5].Value) -band 0xff) -ne 0xff -or
        [int]$mapsSetup.Groups[6].Value -ne 5 -or
        [int]$mapsSetup.Groups[8].Value -ne 6 -or
        [int]$mapsSetup.Groups[9].Value -ne 1) {
        throw "Maps setup did not run in the menu-chain harness.`n$gdbStdout"
    }
    if (-not $mapsTrans.Success -or
        (Convert-MarkerUInt32 $mapsTrans.Groups[1].Value) -ne 0x4d53454c -or
        ((Convert-MarkerUInt32 $mapsTrans.Groups[2].Value) -band 0xff) -ne 0xff -or
        (Convert-MarkerUInt32 $mapsTrans.Groups[4].Value) -ne 0x8000 -or
        [int]$mapsTrans.Groups[5].Value -ne 16 -or
        [int]$mapsTrans.Groups[6].Value -ne 21 -or
        [int]$mapsTrans.Groups[7].Value -ne 21 -or
        [int]$mapsTrans.Groups[8].Value -ne 22 -or
        [int]$mapsTrans.Groups[9].Value -ne 6) {
        throw "Maps did not transition to VSBattle through original A-select logic.`n$gdbStdout"
    }
    if (-not $scene.Success -or [int]$scene.Groups[1].Value -ne 22 -or [int]$scene.Groups[2].Value -ne 21) {
        throw "Final scene state is not VSBattle from Maps.`n$gdbStdout"
    }
    if (-not $task.Success -or [int]$task.Groups[1].Value -ne 1 -or [int]$task.Groups[5].Value -ne 22) {
        throw "Taskman did not park at the VSBattle stub boundary.`n$gdbStdout"
    }
    if (-not $roomTitle.Success -or [int]$roomTitle.Groups[1].Value -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[2].Value) -ne 0 -or (Convert-MarkerUInt32 $roomTitle.Groups[3].Value) -ne 0) {
        throw "Menu-chain harness replayed opening/title work.`n$gdbStdout"
    }

    Write-Output ("Menu-chain VSBattle harness passed: VS->PV mask={0}, PV->Maps mask={1}, Maps->VSBattle mask={2}, final={3}/{4}" -f $vs.Groups[2].Value, $pvTrans.Groups[2].Value, $mapsTrans.Groups[2].Value, $scene.Groups[1].Value, $scene.Groups[2].Value)
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
