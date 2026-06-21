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
$rom = Join-Path $root 'smash64ds-vs-setup.nds'
$elf = Join-Path $root 'smash64ds-vs-setup.elf'
$melonDsPath = Resolve-MelonDSPath -Root $root -MelonDS $MelonDS
$melonDsDir = Split-Path -Parent $melonDsPath
$logDir = Join-Path $root 'artifacts\emulator-logs'
$stdout = Join-Path $logDir 'melonds.vs-setup-harness.stdout.log'
$stderr = Join-Path $logDir 'melonds.vs-setup-harness.stderr.log'
$configState = $null
$emulator = $null
$scriptName = '_vs_setup_harness.gdb'

if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }

& make -C $root TARGET=smash64ds-vs-setup BUILD=build-vs-setup-harness NDS_DEV_SCENE_HARNESS=vs_setup -j4
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (-not (Test-Path $rom) -or -not (Test-Path $elf)) {
    throw 'VS setup harness build did not produce smash64ds-vs-setup.nds and smash64ds-vs-setup.elf.'
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
        'printf "ROOM=%u\n", gNdsOpeningRoomTickCount',
        'printf "TITLE=%#x,%#x\n", gNdsTitleOriginalStartResult, gNdsTitleDrawResult',
        'printf "VS_ORIGINAL=%#x,%#x,%#x,%#x,%#x,%u,%u,%u,%u,%u,%#x\n", gNdsVSModeOriginalStartResult, gNdsVSModeOriginalFuncStartResult, gNdsVSModeOriginalRelocResult, gNdsVSModeOriginalSetupResult, gNdsVSModeOriginalSetupMask, gNdsVSModeOriginalLoadedFileCount, gNdsVSModeOriginalGObjCount, gNdsVSModeOriginalCameraCount, gNdsVSModeOriginalSObjCount, gNdsVSModeOriginalMainGObjID, gNdsVSModeOriginalDeferredMask',
        'printf "VS_STATE=%u,%u,%u,%u,%#x\n", gNdsVSModeOriginalCursorIndex, gNdsVSModeOriginalRule, gNdsVSModeOriginalTime, gNdsVSModeOriginalStock, gNdsVSModeOriginalButtonMask',
        'printf "TASK=%u,%u,%u,%u,%u\n", gNdsTaskmanLoopReached, gNdsTaskmanSceneUpdateSet, gNdsTaskmanSceneDrawSet, gNdsTaskmanControllerAutoRead, gNdsSceneBoundaryKind',
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
    $room = [int](Get-MatchValue $gdbStdout 'ROOM=([0-9]+)')
    $title = [regex]::Match($gdbStdout, 'TITLE=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0)')
    $vs = [regex]::Match($gdbStdout, 'VS_ORIGINAL=(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),(0x[0-9a-fA-F]+|0),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $state = [regex]::Match($gdbStdout, 'VS_STATE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),(0x[0-9a-fA-F]+|0)')
    $task = [regex]::Match($gdbStdout, 'TASK=([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)')

    if (-not $harn.Success -or
        (Convert-MarkerUInt32 $harn.Groups[1].Value) -ne 0x4841524e -or
        [int]$harn.Groups[2].Value -ne 2 -or
        [int]$harn.Groups[3].Value -ne 9 -or
        [int]$harn.Groups[4].Value -ne 1 -or
        (Convert-MarkerUInt32 $harn.Groups[5].Value) -ne 0) {
        throw "VS setup harness marker did not select VS Mode from Title.`n$gdbStdout"
    }
    if (-not $scene.Success -or
        [int]$scene.Groups[1].Value -ne 9 -or
        [int]$scene.Groups[2].Value -ne 1) {
        throw "Live scene state is not VS Mode from Title.`n$gdbStdout"
    }
    if ($room -ne 0) {
        throw "VS setup harness replayed Opening Room before VS setup (room tick $room).`n$gdbStdout"
    }
    if (-not $title.Success -or
        (Convert-MarkerUInt32 $title.Groups[1].Value) -ne 0 -or
        (Convert-MarkerUInt32 $title.Groups[2].Value) -ne 0) {
        throw "VS setup harness unexpectedly ran the Title setup/draw path.`n$gdbStdout"
    }
    if (-not $vs.Success -or
        (Convert-MarkerUInt32 $vs.Groups[1].Value) -ne 0x56535354 -or
        (Convert-MarkerUInt32 $vs.Groups[2].Value) -ne 0x56534653 -or
        (Convert-MarkerUInt32 $vs.Groups[3].Value) -ne 0x5653524c -or
        (Convert-MarkerUInt32 $vs.Groups[4].Value) -ne 0x56535355 -or
        (Convert-MarkerUInt32 $vs.Groups[5].Value) -ne 0x1f -or
        [int]$vs.Groups[6].Value -ne 2 -or
        [int]$vs.Groups[7].Value -lt 8 -or
        [int]$vs.Groups[8].Value -lt 1 -or
        [int]$vs.Groups[9].Value -lt 20 -or
        (Convert-MarkerUInt32 $vs.Groups[11].Value) -ne 0x7) {
        throw "Imported original VS Mode setup did not reach the bounded proof.`n$gdbStdout"
    }
    if (-not $state.Success -or
        [int]$state.Groups[1].Value -ne 0 -or
        [int]$state.Groups[2].Value -lt 0 -or
        [int]$state.Groups[3].Value -eq 0 -or
        [int]$state.Groups[4].Value -eq 0 -or
        (Convert-MarkerUInt32 $state.Groups[5].Value) -ne 0x3f) {
        throw "VS Mode setup state/button proof is incomplete.`n$gdbStdout"
    }
    if (-not $task.Success -or
        [int]$task.Groups[1].Value -ne 1 -or
        [int]$task.Groups[2].Value -ne 1 -or
        [int]$task.Groups[3].Value -ne 1 -or
        [int]$task.Groups[4].Value -ne 1 -or
        [int]$task.Groups[5].Value -ne 9) {
        throw "VS Mode taskman boundary did not park at scene kind 9.`n$gdbStdout"
    }

    Write-Output ("VS setup harness passed: scene={0}/{1} setup={2} files={3} sobj={4} buttons={5}" -f
        $scene.Groups[1].Value,
        $scene.Groups[2].Value,
        $vs.Groups[5].Value,
        $vs.Groups[6].Value,
        $vs.Groups[9].Value,
        $state.Groups[5].Value)
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
