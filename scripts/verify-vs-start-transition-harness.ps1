param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$DelaySeconds = 5
)

$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')

function Get-MatchValue {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Fallback = '0'
    )

    $match = [regex]::Match($Text, $Pattern)
    if ($match.Success) {
        return $match.Groups[1].Value
    }
    return $Fallback
}

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$rom = Join-Path $root 'smash64ds-vs-start.nds'
$elf = Join-Path $root 'smash64ds-vs-start.elf'
$melonDsPath = Resolve-MelonDSPath -Root $root -MelonDS $MelonDS
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Join-Path $root 'artifacts\emulator-logs'
$stdout = Join-Path $logDir 'melonds.vs-start-transition-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.vs-start-transition-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_vs_start_transition_harness.gdb'

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }

& make -C $root TARGET=smash64ds-vs-start BUILD=build-vs-start-harness NDS_DEV_SCENE_HARNESS=vs_start_transition -j4
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'VS Start transition harness build did not produce smash64ds-vs-start.nds and smash64ds-vs-start.elf.'
}
if (-not (Test-Path $melonDsPath)) {
    throw "melonDS executable not found: $melonDsPath"
}
if (-not (Test-Path $Gdb)) {
    throw "GDB executable not found: $Gdb"
}

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
        'printf "BOUNDARY=%#x,%u\n", gNdsSceneBoundaryResult, gNdsSceneBoundaryKind',
        'printf "VS_SETUP=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%#x\n", gNdsVSModeOriginalStartResult, gNdsVSModeOriginalFuncStartResult, gNdsVSModeOriginalRelocResult, gNdsVSModeOriginalSetupResult, gNdsVSModeOriginalSetupMask, gNdsVSModeOriginalLoadedFileCount, gNdsVSModeOriginalGObjCount, gNdsVSModeOriginalCameraCount, gNdsVSModeOriginalSObjCount, gNdsVSModeOriginalButtonMask',
        'printf "VS_STATE=%u,%u,%u,%u,%#x\n", gNdsVSModeOriginalCursorIndex, gNdsVSModeOriginalRule, gNdsVSModeOriginalTime, gNdsVSModeOriginalStock, gNdsVSModeOriginalButtonMask',
        'printf "VS_TRANS=%#x,%#x,%u,%#x,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsVSModeStartTransitionResult, gNdsVSModeStartTransitionMask, gNdsVSModeStartTransitionUpdateCount, gNdsVSModeStartTransitionInputMask, gNdsVSModeStartTransitionScenePrevBefore, gNdsVSModeStartTransitionSceneCurrBefore, gNdsVSModeStartTransitionScenePrevAfterTap, gNdsVSModeStartTransitionSceneCurrAfterTap, gNdsVSModeStartTransitionScenePrevFinal, gNdsVSModeStartTransitionSceneCurrFinal, gNdsVSModeStartTransitionExitInterrupt, gNdsVSModeStartTransitionTaskmanStatus',
        'printf "VS_SAVED=%u,%u,%u,%u,%#x\n", gNdsVSModeStartTransitionSavedRule, gNdsVSModeStartTransitionSavedTime, gNdsVSModeStartTransitionSavedStock, gNdsVSModeStartTransitionCleanupCount, gNdsVSModeStartTransitionButtonMaskAfter',
        'printf "TASK=%u,%u,%u,%u,%u\n", gNdsTaskmanLoopReached, gNdsTaskmanSceneUpdateSet, gNdsTaskmanSceneDrawSet, gNdsTaskmanControllerAutoRead, gNdsSceneBoundaryKind',
        'printf "TITLE=%#x,%#x\n", gNdsTitleOriginalStartResult, gNdsTitleDrawResult',
        'printf "ROOM=%u\n", gNdsOpeningRoomTickCount',
        'detach',
        'quit'
    )

    $gdbResult = Invoke-GdbMarkerScript `
        -Gdb $Gdb `
        -Elf $elf `
        -Root $root `
        -Commands $gdbCommands `
        -ScriptName $scriptName
    $gdbStdout = $gdbResult.Stdout

    $harn = [regex]::Match($gdbStdout, 'HARN=(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $scene = [regex]::Match($gdbStdout, 'SCENE=([0-9]+),([0-9]+)')
    $boundary = [regex]::Match($gdbStdout, 'BOUNDARY=(0x[0-9a-fA-F]+|0),([0-9]+)')
    $vsSetup = [regex]::Match($gdbStdout, 'VS_SETUP=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $state = [regex]::Match($gdbStdout, 'VS_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $trans = [regex]::Match($gdbStdout, 'VS_TRANS=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $saved = [regex]::Match($gdbStdout, 'VS_SAVED=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $task = [regex]::Match($gdbStdout, 'TASK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')
    $title = [regex]::Match($gdbStdout, 'TITLE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $room = [int](Get-MatchValue $gdbStdout 'ROOM=([0-9]+)')

    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 4 -or
        [int]$harn.Groups[3].Value -ne 9 -or
        [int]$harn.Groups[4].Value -ne 1 -or
        (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "VS Start transition harness marker did not select VS Mode from Title.`n$gdbStdout"
    }
    if (-not $vsSetup.Success -or
        (Convert-MarkerUInt32 $vsSetup.Groups[1].Value) -ne 0x56535354 -or
        (Convert-MarkerUInt32 $vsSetup.Groups[2].Value) -ne 0x56534653 -or
        (Convert-MarkerUInt32 $vsSetup.Groups[3].Value) -ne 0x5653524c -or
        (Convert-MarkerUInt32 $vsSetup.Groups[4].Value) -ne 0x56535355 -or
        ((Convert-MarkerUInt32 $vsSetup.Groups[5].Value) -band 0x1f) -ne 0x1f -or
        [int]$vsSetup.Groups[6].Value -ne 2 -or
        [int]$vsSetup.Groups[9].Value -lt 20 -or
        (Convert-MarkerUInt32 $vsSetup.Groups[10].Value) -ne 0x3f) {
        throw "Imported original VS setup proof failed before transition.`n$gdbStdout"
    }
    if (-not $state.Success -or
        [int]$state.Groups[1].Value -ne 0 -or
        [int]$state.Groups[2].Value -ne 0 -or
        [int]$state.Groups[3].Value -ne 3 -or
        [int]$state.Groups[4].Value -ne 2 -or
        (Convert-MarkerUInt32 $state.Groups[5].Value) -ne 0x3f) {
        throw "VS setup state defaults are not the expected VS Start cursor/time defaults.`n$gdbStdout"
    }
    if (-not $trans.Success -or
        (Convert-MarkerUInt32 $trans.Groups[1].Value) -ne 0x56535452 -or
        ((Convert-MarkerUInt32 $trans.Groups[2].Value) -band 0xff) -ne 0xff -or
        [int]$trans.Groups[3].Value -lt 11 -or
        (Convert-MarkerUInt32 $trans.Groups[4].Value) -ne 0x8000 -or
        [int]$trans.Groups[5].Value -ne 1 -or
        [int]$trans.Groups[6].Value -ne 9 -or
        [int]$trans.Groups[7].Value -ne 9 -or
        [int]$trans.Groups[8].Value -ne 16 -or
        [int]$trans.Groups[9].Value -ne 9 -or
        [int]$trans.Groups[10].Value -ne 16 -or
        [int]$trans.Groups[11].Value -ne 1 -or
        [int]$trans.Groups[12].Value -ne 1) {
        throw "Original mnVSModeMain did not prove VS Start -> PlayersVS transition.`n$gdbStdout"
    }
    if (-not $saved.Success -or
        [int]$saved.Groups[1].Value -ne 1 -or
        [int]$saved.Groups[2].Value -ne 3 -or
        [int]$saved.Groups[3].Value -ne 2 -or
        [int]$saved.Groups[4].Value -ne 1 -or
        (Convert-MarkerUInt32 $saved.Groups[5].Value) -ne 0x3f) {
        throw "Original mnVSModeSaveSettings or cleanup proof failed.`n$gdbStdout"
    }
    if (-not $scene.Success -or
        [int]$scene.Groups[1].Value -ne 16 -or
        [int]$scene.Groups[2].Value -ne 9) {
        throw "Final scene state is not PlayersVS from VSMode.`n$gdbStdout"
    }
    if (-not $boundary.Success -or
        (Convert-MarkerUInt32 $boundary.Groups[1].Value) -ne 0x53434e45 -or
        [int]$boundary.Groups[2].Value -ne 16) {
        throw "PlayersVS scene boundary was not reached.`n$gdbStdout"
    }
    if (-not $task.Success -or
        [int]$task.Groups[1].Value -ne 1 -or
        [int]$task.Groups[2].Value -ne 1 -or
        [int]$task.Groups[4].Value -ne 1 -or
        [int]$task.Groups[5].Value -ne 16) {
        throw "Taskman or boundary diagnostics did not settle at PlayersVS.`n$gdbStdout"
    }
    if ($room -ne 0) {
        throw "VS Start transition harness replayed Opening Room (room tick $room).`n$gdbStdout"
    }
    if (-not $title.Success -or
        (Convert-MarkerUInt32 $title.Groups[1].Value) -ne 0 -or
        (Convert-MarkerUInt32 $title.Groups[2].Value) -ne 0) {
        throw "VS Start transition harness unexpectedly ran Title setup/draw.`n$gdbStdout"
    }

    Write-Output ("VS Start transition harness passed: scene={0}/{1} trans={2} mask={3} saved={4}/{5}/{6}" -f
        $scene.Groups[1].Value,
        $scene.Groups[2].Value,
        $trans.Groups[1].Value,
        $trans.Groups[2].Value,
        $saved.Groups[1].Value,
        $saved.Groups[2].Value,
        $saved.Groups[3].Value)
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
