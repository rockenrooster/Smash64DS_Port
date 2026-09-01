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
    # Lock the pair on the SIMULATION CLOCK instead of the presented-frame
    # counter. 0 keeps the presented-frame mode; any other value captures at
    # `gSCManagerBattleState->time_remain == $TimeRemain` and again at
    # `== $TimeRemain - 1`, i.e. two adjacent simulation ticks.
    #
    # This mode exists because the frame mode CANNOT be used to compare two
    # different builds. `time_remain` decrements once per simulation tic and is
    # identical across builds by construction; the presented-frame counter is
    # not, because a faster build presents more frames per tic, so the same
    # frame index names a later moment of the match in the faster arm. R2-02 E8
    # measured exactly that: two ROMs stopped at presented frame 1100 sat two
    # tics apart and their top screens differed on 57% of pixels, which read as
    # a catastrophic regression and was purely the lock. At the `time_remain`
    # lock the same pair was pixel-identical.
    #
    # So: same ROM, adjacent presented frames -> -FirstFrame/-SecondFrame.
    # Two ROMs, same game moment -> -TimeRemain, the same value in both runs.
    #
    # WHAT THIS LOCK DOES NOT REMOVE, measured 2026-08-04 on the two gate-5
    # ROMs. It synchronizes the SIMULATION; the pixels on screen are still the
    # last COMPLETED present, and the core is halted when the shot is taken. At
    # tic 3000 both arms reported EXACT_LOCK 3000/2998 -- the lock works -- yet
    # the cross-build pair differed on 48.36% of the guest viewport, against
    # 30.85% for two frames of the SAME build two tics apart. So at a
    # high-motion moment one present of display quantization (plus a possibly
    # stale halted-core buffer) already dominates any fidelity signal.
    # Therefore: choose a LOW-MOTION tic for a fidelity comparison, and always
    # report the same-build adjacent-present delta beside the cross-build one as
    # the floor. A cross-build number without that floor is not interpretable.
    [ValidateRange(0,3600)][int]$TimeRemain = 0,
    [ValidateRange(0,1)][int]$FoxCpuMode = 1,
    # Same single-session selector surface as capture-melonds.ps1.  Exact-time
    # captures used to drop -SetGlobals entirely, forcing lab A/Bs back onto
    # SELECT presses at different poses.  Keep the writes inside this GDB
    # session so the requested arm is already live before the frame lock.
    [string[]]$GlobalWrites = @(),
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

$lockOnTime = ($TimeRemain -gt 0)
if ($lockOnTime) {
    Assert-Condition ($TimeRemain -gt 1) `
        "-TimeRemain must leave room for the following tic: $TimeRemain."
    # `time_remain` counts DOWN, so the second capture is one lower.
    $lockExpression = 'gSCManagerBattleState->time_remain'
    $firstKey = $TimeRemain
    $secondKey = $TimeRemain - 1
    # `<=`, NOT `==`. The breakpoint sits on ndsBattlePlayableFrameCompleteMarker,
    # which fires once per PRESENTED frame, while `time_remain` decrements once
    # per simulation tic -- and this port runs more than one tic per present. So
    # the sequence observable at the marker SKIPS values, and `== N-1` can simply
    # never be true: the first attempt at this (2026-08-04) captured tic 3000 and
    # then hung waiting for a 2999 that the marker never saw.
    #
    # With `<=` the first stop is the first frame at or past the requested tic
    # and the second is the frame immediately after it, which is the adjacent
    # pair the frame mode gives. The cost is that the tic actually captured may
    # undershoot the request, and two builds presenting at different rates may
    # land on DIFFERENT tics -- so the value is re-read from the marker and
    # printed as EXACT_LOCK below. A cross-build pair is synchronized only if
    # both runs report the same EXACT_LOCK; that is the caller's check to make,
    # and it is why the number is printed rather than merely asserted.
    $lockOperator = '<='
    # Index of time_remain in $markerValues below; the guard re-reads the lock
    # out of the marker rather than trusting the breakpoint condition.
    $lockMarkerIndex = 3
    $lockNoun = 'tic'
} else {
    Assert-Condition ($SecondFrame -eq ($FirstFrame + 1)) `
        "Cut G capture frames must be adjacent: $FirstFrame/$SecondFrame."
    $lockExpression = 'gNdsRendererProfileFrameCount'
    $firstKey = $FirstFrame
    $secondKey = $SecondFrame
    # The presented-frame counter increments exactly once per marker hit, so it
    # never skips and `==` is exact.
    $lockOperator = '=='
    $lockMarkerIndex = 0
    $lockNoun = 'frame'
}
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
$readyFirst = Join-Path $TempDirectory "cut-g-$token.$lockNoun$firstKey.ready"
$goFirst = Join-Path $TempDirectory "cut-g-$token.$lockNoun$firstKey.go"
$readySecond = Join-Path $TempDirectory "cut-g-$token.$lockNoun$secondKey.ready"
$goSecond = Join-Path $TempDirectory "cut-g-$token.$lockNoun$secondKey.go"
$temporaryFiles = @(
    $gdbScript, $gdbStdout, $gdbStderr,
    $readyFirst, $goFirst, $readySecond, $goSecond)
Remove-Item -LiteralPath $temporaryFiles -Force -ErrorAction SilentlyContinue

$readyFirstGdb = $readyFirst.Replace('\', '/')
$goFirstGdb = $goFirst.Replace('\', '/')
$readySecondGdb = $readySecond.Replace('\', '/')
$goSecondGdb = $goSecond.Replace('\', '/')
$markerFormat =
    'CUTG_EXACT=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u'
$markerValues =
    'gNdsRendererProfileFrameCount, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->is_control_disable, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->is_control_disable, gNdsIFCommonNativeOamEnabled, gNdsIFCommonNativeOamFrameRecognizedCalls, gNdsIFCommonNativeOamFrameDrawCalls, gNdsIFCommonNativeOamFrameFallbackCalls, gNdsIFCommonNativeOamFrameSObjCount, gNdsIFCommonNativeOamFrameSemanticHash, gNdsIFCommonNativeOamFrameObjectCount, gNdsIFCommonNativeOamLastFallbackReason, gNdsIFCommonNativeOamFrameCommitCalls, gNdsIFCommonNativeOamFrameIdle, gNdsIFCommonNativeOamHotConvertCount, gNdsIFCommonNativeOamRuntimeUploadBytes, gNdsIFCommonNativeOamPreparePaletteBytes, gNdsIFCommonNativeOamPrepareSuccessCount, gNdsIFCommonNativeOamPrepareFailCount, gNdsIFCommonNativeOamPrepareCloudTextureBytes, gNdsIFCommonNativeOamPrepareCloudTextureCount, gNdsIFCommonNativeOamPrepareBytes, gNdsIFCommonNativeOamPrepareCloudFailureStage, gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[0], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[1], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[2], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[3], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[4], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[5], gNdsIFCommonNativeOamFrameCloudDrawCount'
$selectorCommands = @()
$captureSelectorAssertions = @()
if ($FoxCpuMode -ge 0) {
    $selectorCommands = @(
        'tbreak scVSBattleStartBattle',
        'continue',
        ('set variable gNdsBattlePlayableFoxCpuEnabled = {0}' -f $FoxCpuMode)
    )
}
foreach ($pair in $GlobalWrites) {
    if ([string]::IsNullOrWhiteSpace($pair)) { continue }
    $split = $pair.Split('=', 2)
    Assert-Condition (
        ($split.Count -eq 2) -and
        ($split[0].Trim() -match '^[A-Za-z_][A-Za-z0-9_]*$') -and
        (-not [string]::IsNullOrWhiteSpace($split[1]))) `
        "-GlobalWrites entries must be name=value; got '$pair'."
    $name = $split[0].Trim()
    $value = $split[1].Trim()
    $selectorCommands += @(
        ("set variable {0} = {1}" -f $name, $value),
        ("if {0} != ({1})" -f $name, $value),
        ("echo CUTG_GLOBAL_MISMATCH:{0}\\n" -f $name),
        'quit 1',
        'end'
    )
    # The immediate readback above only proves what GDB wrote to physical
    # memory. ARM9 may retain an older cached value until gameplay. Re-check at
    # each exact capture stop so a screenshot can never claim an arm the guest
    # did not actually consume.
    $captureSelectorAssertions += @(
        ("if {0} != ({1})" -f $name, $value),
        ("echo CUTG_GLOBAL_RUNTIME_MISMATCH:{0}\n" -f $name),
        'quit 1',
        'end'
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
    "tbreak ndsBattlePlayableFrameCompleteMarker if $lockExpression $lockOperator $firstKey",
    'continue'
)
$commands += $captureSelectorAssertions
$commands += @(
    ("printf `"$markerFormat\n`", $markerValues"),
    ("shell powershell.exe -NoProfile -Command `"Set-Content -LiteralPath '$readyFirstGdb' -Value ready; while (-not (Test-Path -LiteralPath '$goFirstGdb')) { Start-Sleep -Milliseconds 25 }`""),
    "tbreak ndsBattlePlayableFrameCompleteMarker if $lockExpression $lockOperator $secondKey",
    'continue'
)
$commands += $captureSelectorAssertions
$commands += @(
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
        -GdbProcess $gdbProcess -EmulatorProcess $emulator -Frame $firstKey
    Save-ExactFrameWindowCapture -Handle $handle -Path $outputPath
    Set-Content -LiteralPath $goFirst -Value go

    Wait-ExactCaptureReady -ReadyPath $readySecond `
        -GdbProcess $gdbProcess -EmulatorProcess $emulator -Frame $secondKey
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
    $expectedFrames = @($firstKey, $secondKey)
    for ($i = 0; $i -lt 2; $i++) {
        $row = $rows[$i]
        Assert-Condition ($row.Count -eq 33) `
            "Exact Cut G $lockNoun $($expectedFrames[$i]) marker had $($row.Count) fields." `
            $gdbText
        # Re-read the lock from the marker instead of trusting that the
        # breakpoint condition is the thing that fired: in time mode the
        # capture's whole value is that both arms stopped at the SAME tic, and
        # an unverified stop is exactly the failure this mode exists to prevent.
        if ($lockOnTime) {
            # `<=` can undershoot by the number of tics one presented frame
            # advances. Bound it rather than pinning it: a stop far past the
            # request means the breakpoint condition was already true on entry,
            # i.e. the run reached the battle later than the requested tic and
            # the capture is not the moment that was asked for.
            Assert-Condition (
                ($row[$lockMarkerIndex] -le $expectedFrames[$i]) -and
                ($row[$lockMarkerIndex] -gt ($expectedFrames[$i] - 16))) `
                ("Exact Cut G stopped at $lockNoun $($row[$lockMarkerIndex]), " +
                 "which is not within 16 of the requested $($expectedFrames[$i]). " +
                 'Pick a tic the run actually passes through while the battle ' +
                 'timer is running.') `
                $gdbText
        } else {
            Assert-Condition ($row[$lockMarkerIndex] -eq $expectedFrames[$i]) `
                ("Exact Cut G stopped at $lockNoun $($row[$lockMarkerIndex]), " +
                 "expected $($expectedFrames[$i]).") `
                $gdbText
        }
        Assert-Condition (
            $row[1] -eq 1 -and $row[2] -eq 1 -and
            $row[3] -gt 0 -and $row[4] -gt 0 -and
            ($row[3] + $row[4]) -eq 3600 -and
            $row[5] -eq 0 -and $row[6] -eq 0) `
            "Exact $lockNoun $($expectedFrames[$i]) was not source GO with a running one-minute timer and unlocked fighters." `
            $gdbText
        # Always true, GO presenting or not: the native OAM path owns the
        # overlay, never falls back, and never converts or uploads at runtime.
        Assert-Condition (
            $row[7] -eq 1 -and $row[10] -eq 0 -and $row[14] -eq 0 -and
            $row[17] -eq 0 -and $row[18] -eq 0 -and
            $row[20] -eq 1 -and $row[21] -eq 0 -and $row[25] -eq 0 -and
            $row[26] -gt 0 -and $row[27] -gt 0 -and
            $row[28] -gt 0 -and $row[29] -gt 0 -and
            $row[30] -gt 0 -and $row[31] -gt 0) `
            "Exact $lockNoun $($expectedFrames[$i]) lost native-OAM ownership, no-fallback, or no-conversion state." `
            $gdbText
        # The rest is the GO overlay's own census and only means anything while
        # it is presenting. Decide that from THIS frame's recognition/draw
        # counters, not FrameIdle: the exact marker runs before NativeOamCommit,
        # while BeginFrame has already reset FrameIdle to zero. Using row 16
        # therefore mislabeled every mid-match pre-commit stop as active GO and
        # threw after writing otherwise-valid screenshots. The prepare-byte
        # counts below are cumulative GO-phase constants.
        if (($row[8] -ne 0) -or ($row[9] -ne 0)) {
            Assert-Condition (
                $row[8] -eq 2 -and $row[9] -eq 2 -and $row[11] -eq 13 -and
                $row[12] -ne 0x49464f41 -and $row[13] -eq 23 -and
                $row[15] -eq 1 -and $row[19] -eq 32 -and
                $row[22] -eq 65536 -and $row[23] -eq 2 -and
                $row[24] -eq 41728 -and $row[32] -eq 2) `
                "Exact $lockNoun $($expectedFrames[$i]) lost native-OAM GO recognition or drawing state." `
                $gdbText
        } else {
            Assert-Condition (
                $row[8] -eq 0 -and $row[9] -eq 0 -and $row[13] -eq 0 -and
                $row[15] -eq 0 -and $row[32] -eq 0) `
                "Exact $lockNoun $($expectedFrames[$i]) had inconsistent inactive GO counters." `
                $gdbText
        }
    }
    # The GO SObjs animate position/scale/alpha, so adjacent source frames may
    # legitimately have different semantic hashes. Their OAM-object census and
    # recognition-derived presentation phase must remain stable. FrameIdle is
    # deliberately excluded for the same pre-commit ordering reason above.
    $firstGoActive = ($rows[0][8] -ne 0) -or ($rows[0][9] -ne 0)
    $secondGoActive = ($rows[1][8] -ne 0) -or ($rows[1][9] -ne 0)
    Assert-Condition (
        $rows[0][13] -eq $rows[1][13] -and
        $firstGoActive -eq $secondGoActive) `
        'Exact Cut G pair crossed an OAM-object-count or GO presentation transition.' `
        $gdbText
    Assert-Condition ((Test-Path -LiteralPath $outputPath -PathType Leaf) -and
        (Test-Path -LiteralPath $secondOutputPath -PathType Leaf)) `
        'Exact Cut G synchronization completed without both screenshot files.'

    # Name the lock AND the value actually reached. In time mode the request is
    # a floor, not a pin, so two runs are synchronized only if these agree --
    # print it in a greppable form rather than leaving the caller to infer it.
    Write-Output (
        "EXACT_LOCK=$lockExpression,$($rows[0][$lockMarkerIndex])," +
        "$($rows[1][$lockMarkerIndex])")
    Write-Output (
        "Captured exact Cut G ${lockNoun}s $($rows[0][$lockMarkerIndex])/" +
        "$($rows[1][$lockMarkerIndex]) (requested $firstKey/${secondKey}, " +
        "lock $lockExpression): '$outputPath', '$secondOutputPath'.")
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
