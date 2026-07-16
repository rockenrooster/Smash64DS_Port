[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Gdb,
    [Parameter(Mandatory=$true)][string]$Elf,
    [ValidateRange(1,65535)][int]$GdbPort,
    [Parameter(Mandatory=$true)][int]$EmulatorProcessId,
    [Parameter(Mandatory=$true)][long]$WindowHandle,
    [Parameter(Mandatory=$true)][string]$Output,
    [Parameter(Mandatory=$true)][string]$SecondOutput,
    [ValidateRange(1,1000000)][int]$FirstFrame = 200,
    [ValidateRange(1,1000000)][int]$SecondFrame = 201,
    [ValidateRange(0,1)][int]$FoxCpuMode = 1,
    [string]$TempDirectory = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$visibilityDirectory = Join-Path $root 'artifacts\visibility'

function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context = '')
    if (-not $Condition) {
        if ([string]::IsNullOrWhiteSpace($Context)) { throw $Message }
        throw "$Message`n$Context"
    }
}

function Resolve-VisibilityOutput {
    param([string]$Path)

    $resolved = if ([System.IO.Path]::IsPathRooted($Path)) {
        [System.IO.Path]::GetFullPath($Path)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $root $Path))
    }
    $visibilityPrefix = [System.IO.Path]::GetFullPath(
        $visibilityDirectory).TrimEnd('\', '/') +
        [System.IO.Path]::DirectorySeparatorChar
    Assert-Condition ($resolved.StartsWith(
        $visibilityPrefix, [System.StringComparison]::OrdinalIgnoreCase)) `
        "Exact Cut G captures must stay under '$visibilityDirectory': $resolved"
    return $resolved
}

function Convert-MarkerField {
    param([string]$Value)

    if ($Value -match '^0x[0-9a-fA-F]+$') {
        return [int64][uint32]::Parse(
            $Value.Substring(2),
            [System.Globalization.NumberStyles]::HexNumber)
    }
    return [int64]$Value
}

function Get-MarkerRows {
    param([string]$Text)

    foreach ($match in [regex]::Matches(
        $Text, '(?m)^CUTG_EXACT=([^\r\n]+)')) {
        $row = [int64[]]@($match.Groups[1].Value.Split(',') |
            ForEach-Object { Convert-MarkerField $_ })
        Write-Output -NoEnumerate $row
    }
}

function Wait-ExactCaptureReady {
    param(
        [string]$ReadyPath,
        [System.Diagnostics.Process]$GdbProcess,
        [System.Diagnostics.Process]$EmulatorProcess,
        [int]$Frame
    )

    $deadline = (Get-Date).AddSeconds(120)
    while (-not (Test-Path -LiteralPath $ReadyPath -PathType Leaf)) {
        $GdbProcess.Refresh()
        $EmulatorProcess.Refresh()
        if ($GdbProcess.HasExited) {
            throw "GDB exited before exact Cut G frame $Frame was ready."
        }
        if ($EmulatorProcess.HasExited) {
            throw "melonDS exited before exact Cut G frame $Frame was ready."
        }
        if ((Get-Date) -ge $deadline) {
            throw "Timed out waiting for exact Cut G frame $Frame."
        }
        Start-Sleep -Milliseconds 25
    }
}

Add-Type -AssemblyName System.Drawing
if ($null -eq ('Smash64DSCutGExactCapture' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSCutGExactCapture
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);
}
'@
}

function Save-ExactFrameWindowCapture {
    param([System.IntPtr]$Handle, [string]$Path)

    $rect = New-Object Smash64DSCutGExactCapture+Rect
    Assert-Condition `
        ([Smash64DSCutGExactCapture]::GetWindowRect($Handle, [ref]$rect)) `
        'Could not read the exact-frame melonDS window bounds.'
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    Assert-Condition ($width -gt 0 -and $height -gt 0) `
        "Invalid exact-frame melonDS capture bounds ${width}x${height}."

    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen(
            $rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

Assert-Condition ($SecondFrame -eq ($FirstFrame + 1)) `
    "Cut G capture frames must be adjacent: $FirstFrame/$SecondFrame."
Assert-Condition ($FoxCpuMode -eq 1) `
    'Exact natural GO capture requires FoxCpuMode 1.'
$gdbPath = (Resolve-Path -LiteralPath $Gdb).Path
$elfPath = (Resolve-Path -LiteralPath $Elf).Path
$outputPath = Resolve-VisibilityOutput $Output
$secondOutputPath = Resolve-VisibilityOutput $SecondOutput
New-Item -ItemType Directory -Force -Path $visibilityDirectory | Out-Null

if ([string]::IsNullOrWhiteSpace($TempDirectory)) {
    $TempDirectory = Join-Path $root 'artifacts\verifier-temp\cut-g-capture'
} elseif (-not [System.IO.Path]::IsPathRooted($TempDirectory)) {
    $TempDirectory = Join-Path $root $TempDirectory
}
New-Item -ItemType Directory -Force -Path $TempDirectory | Out-Null

$emulator = Get-Process -Id $EmulatorProcessId -ErrorAction Stop
Assert-Condition (-not $emulator.HasExited) `
    'melonDS exited before exact Cut G synchronization began.'
$handle = [System.IntPtr]$WindowHandle
Assert-Condition ($handle -ne [System.IntPtr]::Zero) `
    'Exact Cut G capture received an empty melonDS window handle.'

$token = 'p{0}-{1}' -f $PID, ([System.Guid]::NewGuid().ToString('N'))
$gdbScript = Join-Path $TempDirectory "cut-g-$token.gdb"
$gdbStdout = Join-Path $TempDirectory "cut-g-$token.gdb.stdout.log"
$gdbStderr = Join-Path $TempDirectory "cut-g-$token.gdb.stderr.log"
$readyFirst = Join-Path $TempDirectory "cut-g-$token.frame$FirstFrame.ready"
$goFirst = Join-Path $TempDirectory "cut-g-$token.frame$FirstFrame.go"
$readySecond = Join-Path $TempDirectory "cut-g-$token.frame$SecondFrame.ready"
$goSecond = Join-Path $TempDirectory "cut-g-$token.frame$SecondFrame.go"
$temporaryFiles = @(
    $gdbScript, $gdbStdout, $gdbStderr,
    $readyFirst, $goFirst, $readySecond, $goSecond)
Remove-Item -LiteralPath $temporaryFiles -Force -ErrorAction SilentlyContinue

$readyFirstGdb = $readyFirst.Replace('\', '/')
$goFirstGdb = $goFirst.Replace('\', '/')
$readySecondGdb = $readySecond.Replace('\', '/')
$goSecondGdb = $goSecond.Replace('\', '/')
$markerFormat =
    'CUTG_EXACT=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%u'
$markerValues =
    'gNdsRendererProfileFrameCount, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->is_control_disable, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->is_control_disable, gNdsIFCommonNativeOamEnabled, gNdsIFCommonNativeOamFrameRecognizedCalls, gNdsIFCommonNativeOamFrameDrawCalls, gNdsIFCommonNativeOamFrameFallbackCalls, gNdsIFCommonNativeOamFrameSObjCount, gNdsIFCommonNativeOamFrameSemanticHash, gNdsIFCommonNativeOamFrameObjectCount, gNdsIFCommonNativeOamLastFallbackReason, gNdsIFCommonNativeOamFrameCommitCalls, gNdsIFCommonNativeOamFrameIdle, gNdsIFCommonNativeOamHotConvertCount, gNdsIFCommonNativeOamRuntimeUploadBytes, gNdsIFCommonNativeOamPreparePaletteBytes'
$selectorCommands = @()
if ($FoxCpuMode -ge 0) {
    $selectorCommands = @(
        'tbreak scVSBattleStartBattle',
        'continue',
        ('set variable gNdsBattlePlayableFoxCpuEnabled = {0}' -f $FoxCpuMode)
    )
}
$commands = @(
    'set pagination off',
    'set confirm off',
    'set remotetimeout 10',
    "target remote 127.0.0.1:$GdbPort"
)
$commands += $selectorCommands
$commands += @(
    "tbreak ndsBattlePlayableFrameCompleteMarker if gNdsRendererProfileFrameCount == $FirstFrame",
    'continue',
    ("printf `"$markerFormat\n`", $markerValues"),
    ("shell powershell.exe -NoProfile -Command `"Set-Content -LiteralPath '$readyFirstGdb' -Value ready; while (-not (Test-Path -LiteralPath '$goFirstGdb')) { Start-Sleep -Milliseconds 25 }`""),
    "tbreak ndsBattlePlayableFrameCompleteMarker if gNdsRendererProfileFrameCount == $SecondFrame",
    'continue',
    ("printf `"$markerFormat\n`", $markerValues"),
    ("shell powershell.exe -NoProfile -Command `"Set-Content -LiteralPath '$readySecondGdb' -Value ready; while (-not (Test-Path -LiteralPath '$goSecondGdb')) { Start-Sleep -Milliseconds 25 }`""),
    'detach',
    'quit'
)
Set-Content -LiteralPath $gdbScript -Value ($commands -join "`n")

$gdbProcess = $null
$gdbText = ''
try {
    $gdbProcess = Start-Process -FilePath $gdbPath `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elfPath) `
        -RedirectStandardOutput $gdbStdout `
        -RedirectStandardError $gdbStderr `
        -WindowStyle Hidden -PassThru

    Wait-ExactCaptureReady -ReadyPath $readyFirst `
        -GdbProcess $gdbProcess -EmulatorProcess $emulator -Frame $FirstFrame
    Save-ExactFrameWindowCapture -Handle $handle -Path $outputPath
    Set-Content -LiteralPath $goFirst -Value go

    Wait-ExactCaptureReady -ReadyPath $readySecond `
        -GdbProcess $gdbProcess -EmulatorProcess $emulator -Frame $SecondFrame
    Save-ExactFrameWindowCapture -Handle $handle -Path $secondOutputPath
    Set-Content -LiteralPath $goSecond -Value go

    Assert-Condition ($gdbProcess.WaitForExit(30000)) `
        'Timed out waiting for exact Cut G GDB synchronization to finish.'
    $gdbProcess.WaitForExit()
    $gdbText = ((Get-Content $gdbStdout, $gdbStderr -Raw `
        -ErrorAction SilentlyContinue) -join "`n").Trim()
    Assert-Condition ($gdbProcess.ExitCode -eq 0) `
        "Exact Cut G GDB synchronization failed with exit $($gdbProcess.ExitCode)." `
        $gdbText

    $rows = @(Get-MarkerRows $gdbText)
    Assert-Condition ($rows.Count -eq 2) `
        "Exact Cut G capture received $($rows.Count) of 2 state markers." `
        $gdbText
    $expectedFrames = @($FirstFrame, $SecondFrame)
    for ($i = 0; $i -lt 2; $i++) {
        $row = $rows[$i]
        Assert-Condition ($row.Count -eq 20) `
            "Exact Cut G frame $($expectedFrames[$i]) marker had $($row.Count) fields." `
            $gdbText
        Assert-Condition (
            $row[0] -eq $expectedFrames[$i] -and
            $row[1] -eq 1 -and $row[2] -eq 1 -and
            $row[3] -gt 0 -and $row[4] -gt 0 -and
            ($row[3] + $row[4]) -eq 3600 -and
            $row[5] -eq 0 -and $row[6] -eq 0) `
            "Exact frame $($expectedFrames[$i]) was not source GO with a running one-minute timer and unlocked fighters." `
            $gdbText
        Assert-Condition (
            $row[7] -eq 1 -and $row[8] -eq 2 -and $row[9] -eq 2 -and
            $row[10] -eq 0 -and $row[11] -eq 13 -and
            $row[12] -ne 0x49464f41 -and $row[13] -eq 42 -and
            $row[14] -eq 0 -and $row[15] -eq 1 -and $row[16] -eq 0 -and
            $row[17] -eq 0 -and $row[18] -eq 0 -and $row[19] -eq 0) `
            "Exact frame $($expectedFrames[$i]) lost native-OAM GO recognition, drawing, or no-conversion state." `
            $gdbText
    }
    # The GO SObjs animate position/scale/alpha, so adjacent source frames may
    # legitimately have different semantic hashes. Their exact recognized-call,
    # SObj, and OAM-object census must remain in the same GO presentation phase.
    Assert-Condition ($rows[0][13] -eq $rows[1][13]) `
        'Exact Cut G pair crossed an OAM-object-count transition.' `
        $gdbText
    Assert-Condition ((Test-Path -LiteralPath $outputPath -PathType Leaf) -and
        (Test-Path -LiteralPath $secondOutputPath -PathType Leaf)) `
        'Exact Cut G synchronization completed without both screenshot files.'

    Write-Output (
        "Captured exact Cut G completed frames $FirstFrame/${SecondFrame}: " +
        "'$outputPath', '$secondOutputPath'.")
} finally {
    # Release either debugger-side wait before terminating GDB on an error.
    Set-Content -LiteralPath $goFirst -Value go -ErrorAction SilentlyContinue
    Set-Content -LiteralPath $goSecond -Value go -ErrorAction SilentlyContinue
    if ($null -ne $gdbProcess) {
        $gdbProcess.Refresh()
        if (-not $gdbProcess.HasExited) {
            if (-not $gdbProcess.WaitForExit(2000)) {
                Stop-Process -Id $gdbProcess.Id -Force
                $gdbProcess.WaitForExit()
            }
        }
    }
    Remove-Item -LiteralPath $gdbScript, $readyFirst, $goFirst,
        $readySecond, $goSecond -Force -ErrorAction SilentlyContinue
}
