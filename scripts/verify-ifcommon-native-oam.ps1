param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [ValidateRange(1,65535)][int]$GdbPort = 4463,
    [ValidateRange(-1,127)][int]$RunnerSlot = 2,
    [switch]$NoBuild,
    [ValidateRange(0,1)][int]$HybridOamMode = 0,
    [string]$Target = 'smash64ds-battle-playable-coarse-hwtri',
    [string]$Build = 'build-battle-playable-coarse-hwtri-harness'
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (($RunnerSlot -ne 2) -or ($GdbPort -ne 4463)) {
    throw 'IFCommon native-OAM verification is fail-closed to automation-owned runner slot 2 on ARM9/ARM7 ports 4463/4464.'
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$visibilityDir = Join-Path $root 'artifacts\visibility'
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$tempDir = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot

function Assert-Condition {
    param([bool]$Condition, [string]$Message, [string]$Context = '')
    if (-not $Condition) {
        if ([string]::IsNullOrWhiteSpace($Context)) { throw $Message }
        throw "$Message`n$Context"
    }
}

function Convert-Field {
    param([string]$Value)
    if ($Value -match '^0x[0-9a-fA-F]+$') {
        return [int64][uint32]::Parse(
            $Value.Substring(2),
            [System.Globalization.NumberStyles]::HexNumber)
    }
    return [int64]$Value
}

function Get-MarkerRows {
    param([string]$Text, [string]$Name)
    foreach ($match in [regex]::Matches(
        $Text, '(?m)^' + [regex]::Escape($Name) + '=([^\r\n]+)')) {
        $row = [int64[]]@($match.Groups[1].Value.Split(',') |
            ForEach-Object { Convert-Field $_ })
        Write-Output -NoEnumerate $row
    }
}

function Get-Median {
    param([int64[]]$Values)
    $ordered = @($Values | Sort-Object)
    $middle = [int]($ordered.Count / 2)
    if (($ordered.Count % 2) -eq 0) {
        return [int64](($ordered[$middle - 1] + $ordered[$middle]) / 2)
    }
    return [int64]$ordered[$middle]
}

function Get-Percentile95 {
    param([int64[]]$Values)
    $ordered = @($Values | Sort-Object)
    $index = [Math]::Max(0, [Math]::Ceiling($ordered.Count * 0.95) - 1)
    return [int64]$ordered[$index]
}

function Get-Delta {
    param(
        [object[]]$Baseline,
        [object[]]$Rows,
        [int]$Index,
        [int]$BaselineIndex = $Index
    )
    $values = @()
    $previous = [int64]$Baseline[$BaselineIndex]
    foreach ($row in $Rows) {
        $current = [int64]$row[$Index]
        $values += $current - $previous
        $previous = $current
    }
    return [int64[]]$values
}

function Get-TomlSetting {
    param([string]$Text, [string]$Section, [string]$Key)
    $sectionPattern = '(?ms)^\[' + [regex]::Escape($Section) +
        '\]\s*.*?(?=^\[|\z)'
    $sectionMatch = [regex]::Match($Text, $sectionPattern)
    if (-not $sectionMatch.Success) { return $null }
    $keyMatch = [regex]::Match(
        $sectionMatch.Value,
        '(?m)^' + [regex]::Escape($Key) + '\s*=\s*(.*?)\s*$')
    if (-not $keyMatch.Success) { return $null }
    return $keyMatch.Groups[1].Value
}

function Assert-RunnerReleased {
    $deadline = (Get-Date).AddSeconds(5)
    do {
        $slotProcesses = @()
        foreach ($process in @(Get-Process -Name 'melonDS' `
            -ErrorAction SilentlyContinue)) {
            try {
                if ([System.IO.Path]::GetFullPath($process.Path) -eq
                    [System.IO.Path]::GetFullPath($context.MelonDSPath)) {
                    $slotProcesses += $process
                }
            } catch {
                # A process that exits during inspection is already released.
            }
        }
        $listeners = @(Get-NetTCPConnection -State Listen `
            -ErrorAction SilentlyContinue | Where-Object {
                $_.LocalPort -eq 4463 -or $_.LocalPort -eq 4464
            })
        if (($slotProcesses.Count -eq 0) -and ($listeners.Count -eq 0)) {
            return
        }
        Start-Sleep -Milliseconds 100
    } while ((Get-Date) -lt $deadline)
    $processText = @($slotProcesses | ForEach-Object { $_.Id }) -join ','
    $listenerText = @($listeners | ForEach-Object {
        "$($_.LocalAddress):$($_.LocalPort)/pid$($_.OwningProcess)"
    }) -join ','
    throw "Runner slot 2 cleanup failed: processes=$processText listeners=$listenerText"
}

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
if ($null -eq ('Smash64DSIFCommonCapture' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSIFCommonCapture
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
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(
        IntPtr window, IntPtr insertAfter, int x, int y, int width, int height,
        uint flags);
}
'@
}

function Set-WindowCapturePosition {
    param([System.IntPtr]$WindowHandle)
    [void][Smash64DSIFCommonCapture]::ShowWindow($WindowHandle, 9)
    [void][Smash64DSIFCommonCapture]::SetWindowPos(
        $WindowHandle, [IntPtr](-1),
        $script:MelonDSCanonicalWindowX,
        $script:MelonDSCanonicalWindowY,
        $script:MelonDSCanonicalWindowWidth,
        $script:MelonDSCanonicalWindowHeight, 0x40)
    [void][Smash64DSIFCommonCapture]::SetForegroundWindow($WindowHandle)
    Start-Sleep -Milliseconds 100
}

function Save-WindowCapture {
    param([System.IntPtr]$WindowHandle, [string]$Path)
    $rect = New-Object Smash64DSIFCommonCapture+Rect
    Assert-Condition `
        ([Smash64DSIFCommonCapture]::GetWindowRect($WindowHandle, [ref]$rect)) `
        'Could not read the melonDS window bounds.'
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    Assert-Condition ($width -gt 0 -and $height -gt 0) `
        "Invalid melonDS capture bounds ${width}x${height}."
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Get-BitmapRegionMetrics {
    param(
        [string]$Path,
        [string]$ClearPath,
        [System.Drawing.Rectangle]$Region
    )
    $bitmap = [System.Drawing.Bitmap]::FromFile($Path)
    $clear = [System.Drawing.Bitmap]::FromFile($ClearPath)
    try {
        Assert-Condition ($bitmap.Width -eq 416 -and $bitmap.Height -eq 664) `
            "Capture '$Path' is not the canonical 416x664 window."
        Assert-Condition ($clear.Width -eq $bitmap.Width -and
            $clear.Height -eq $bitmap.Height) `
            "Clear capture '$ClearPath' does not match '$Path'."
        Assert-Condition ($Region.X -ge 0 -and $Region.Y -ge 0 -and
            $Region.Right -le $bitmap.Width -and
            $Region.Bottom -le $bitmap.Height) `
            "Region $Region is outside capture '$Path'."

        $bytes = [byte[]]::new($Region.Width * $Region.Height * 3)
        $byteIndex = 0
        $blue = 0
        $red = 0
        $amber = 0
        $cream = 0
        $dark = 0
        $different = 0
        for ($y = $Region.Y; $y -lt $Region.Bottom; $y++) {
            for ($x = $Region.X; $x -lt $Region.Right; $x++) {
                $pixel = $bitmap.GetPixel($x, $y)
                $clearPixel = $clear.GetPixel($x, $y)
                $bytes[$byteIndex++] = $pixel.R
                $bytes[$byteIndex++] = $pixel.G
                $bytes[$byteIndex++] = $pixel.B
                if ($pixel.ToArgb() -ne $clearPixel.ToArgb()) { $different++ }
                if ($pixel.B -ge $pixel.R + 16 -and
                    $pixel.B -ge $pixel.G + 8) { $blue++ }
                if ($pixel.R -ge $pixel.G + 24 -and
                    $pixel.R -ge $pixel.B + 24) { $red++ }
                if ($pixel.R -ge 160 -and $pixel.G -ge 70 -and
                    $pixel.R -ge $pixel.G + 24 -and
                    $pixel.G -ge $pixel.B + 16) { $amber++ }
                if ($pixel.R -ge 190 -and $pixel.G -ge 150 -and
                    $pixel.B -ge 90) { $cream++ }
                if ($pixel.R -lt 64 -and $pixel.G -lt 64 -and
                    $pixel.B -lt 64) { $dark++ }
            }
        }
        $sha256 = [System.Security.Cryptography.SHA256]::Create()
        try {
            $hash = -join @($sha256.ComputeHash($bytes) |
                ForEach-Object { $_.ToString('x2') })
        } finally {
            $sha256.Dispose()
        }
        return [pscustomobject]@{
            Hash = $hash
            Blue = $blue
            Red = $red
            Amber = $amber
            Cream = $cream
            Dark = $dark
            Different = $different
        }
    } finally {
        $clear.Dispose()
        $bitmap.Dispose()
    }
}

function Save-NearestNeighborCrop {
    param(
        [string]$SourcePath,
        [string]$DestinationPath,
        [System.Drawing.Rectangle]$Region,
        [ValidateRange(1,8)][int]$Scale = 3
    )
    $source = [System.Drawing.Bitmap]::FromFile($SourcePath)
    $crop = $source.Clone($Region, $source.PixelFormat)
    $zoom = New-Object System.Drawing.Bitmap `
        ($Region.Width * $Scale), ($Region.Height * $Scale)
    $graphics = [System.Drawing.Graphics]::FromImage($zoom)
    try {
        $graphics.CompositingMode =
            [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
        $graphics.InterpolationMode =
            [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
        $graphics.PixelOffsetMode =
            [System.Drawing.Drawing2D.PixelOffsetMode]::Half
        $graphics.SmoothingMode =
            [System.Drawing.Drawing2D.SmoothingMode]::None
        $destination = New-Object System.Drawing.Rectangle `
            0, 0, $zoom.Width, $zoom.Height
        $graphics.DrawImage(
            $crop, $destination, 0, 0, $crop.Width, $crop.Height,
            [System.Drawing.GraphicsUnit]::Pixel)
        $zoom.Save($DestinationPath, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $zoom.Dispose()
        $crop.Dispose()
        $source.Dispose()
    }
}

function Invoke-IFCommonRun {
    param(
        [ValidateSet(0,1)][int]$NativeEnabled,
        [string]$CapturePath,
        [string]$ClearCapturePath
    )

    $name = if ($NativeEnabled -ne 0) { 'native' } else { 'fallback' }
    $stdout = Join-Path $logDir "ifcommon-oam-$name.gdb.stdout.log"
    $stderr = Join-Path $logDir "ifcommon-oam-$name.gdb.stderr.log"
    $melonStdout = Join-Path $logDir "ifcommon-oam-$name.melonds.stdout.log"
    $melonStderr = Join-Path $logDir "ifcommon-oam-$name.melonds.stderr.log"
    $gdbScript = Join-Path $tempDir "ifcommon-oam-$name.gdb"
    $ready = Join-Path $tempDir "ifcommon-oam-$name.capture-ready"
    $go = Join-Path $tempDir "ifcommon-oam-$name.capture-go"
    $clearReady = Join-Path $tempDir "ifcommon-oam-$name.clear-ready"
    $clearGo = Join-Path $tempDir "ifcommon-oam-$name.clear-go"
    Remove-Item $stdout, $stderr, $melonStdout, $melonStderr, $gdbScript,
        $ready, $go, $clearReady, $clearGo -Force -ErrorAction SilentlyContinue
    $runConfigHash =
        (Get-FileHash -LiteralPath $configState.Config -Algorithm SHA256).Hash

    $gdbReady = $ready.Replace('\', '/')
    $gdbGo = $go.Replace('\', '/')
    $gdbClearReady = $clearReady.Replace('\', '/')
    $gdbClearGo = $clearGo.Replace('\', '/')
    $activeFormat =
        'IFCOMMON_ACTIVE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u'
    $activeValues =
        'gNdsIFCommonNativeOamEnabled, gNdsRendererProfileFrameCount, gNdsRendererProfileLogicTick, gNdsIFCommonNativeOamFrameBeginTicks, gNdsIFCommonNativeOamFrameTicks, gNdsIFCommonNativeOamFrameCommitTicks, gNdsIFCommonNativeOamFrameCommitCalls, gNdsIFCommonNativeOamFrameClearedObjects, gNdsIFCommonNativeOamFrameIdle, gNdsIFCommonNativeOamFrameRecognizedCalls, gNdsIFCommonNativeOamFrameDrawCalls, gNdsIFCommonNativeOamFrameFallbackCalls, gNdsIFCommonNativeOamFrameSObjCount, gNdsIFCommonNativeOamFrameObjectCount, gNdsIFCommonNativeOamFrameSemanticHash, gNdsIFCommonNativeOamLastFallbackReason, gNdsIFCommonNativeOamCommitCount, gNdsRendererProfilePostVBlankTicks, gNdsRendererProfileForegroundTicks, gNdsOriginalSpriteBg3ClearBytes, gNdsOriginalSpriteBg3CopyBytes, gNdsOriginalSpriteBg3FinalWriteBytes, gNdsSObjForegroundStagingClearBytes, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->is_control_disable, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->is_control_disable, gNdsIFCommonHUDLowerRouteCount, gNdsIFCommonHUDLowerTimerRouteCount, gNdsIFCommonHUDTopGenericPassCount, gNdsIFCommonNativeOamHotConvertCount, gNdsIFCommonNativeOamRuntimeUploadBytes, gNdsIFCommonNativeOamPreparePaletteBytes, gNdsIFCommonNativeOamPrepareCount, gNdsIFCommonNativeOamPrepareSuccessCount'
    $tailFormat =
        'IFCOMMON_TAIL=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%u,%u'
    $tailValues =
        'gNdsIFCommonNativeOamEnabled, gNdsRendererProfileFrameCount, gNdsIFCommonNativeOamFrameBeginTicks, gNdsIFCommonNativeOamFrameTicks, gNdsIFCommonNativeOamFrameCommitTicks, gNdsIFCommonNativeOamFrameCommitCalls, gNdsIFCommonNativeOamFrameClearedObjects, gNdsIFCommonNativeOamFrameIdle, gNdsIFCommonNativeOamFrameRecognizedCalls, gNdsIFCommonNativeOamFrameDrawCalls, gNdsIFCommonNativeOamFrameFallbackCalls, gNdsIFCommonNativeOamFrameSObjCount, gNdsIFCommonNativeOamFrameObjectCount, gNdsIFCommonNativeOamFrameSemanticHash, gNdsIFCommonNativeOamLastFallbackReason, gNdsIFCommonNativeOamCommitCount, gSCManagerBattleState->game_status, sIFCommonTimerIsStarted, gSCManagerBattleState->time_remain, gSCManagerBattleState->time_passed, ((FTStruct *)gGCCommonLinks[3]->user_data.p)->is_control_disable, ((FTStruct *)gGCCommonLinks[3]->link_next->user_data.p)->is_control_disable, gNdsRendererProfilePostVBlankTicks'
    $goFormat = 'IFCOMMON_GO=' +
        ((1..48 | ForEach-Object { '%u' }) -join ',')
    $goValues =
        'gNdsRendererProfileFrameCount, gSCManagerBattleState->game_status, gNdsIFCommonNativeOamFrameCloudDrawCount, gNdsRendererIFCommonCloudQueuedCount, gNdsRendererIFCommonCloudEmittedCount, gNdsIFCommonNativeOamPrepareCloudFailureStage, sNdsIFCommonCloudTextureNames[0], sNdsIFCommonCloudTextureNames[1], sNdsIFCommonTrafficTextureName, gNdsIFCommonNativeOamLastFallbackReason, gNdsIFCommonNativeOamFrameDrawCalls, gNdsIFCommonNativeOamFrameFallbackCalls, gNdsIFCommonNativeOamFrameObjectCount, gNdsIFCommonNativeOamFrameSObjCount, gNdsIFCommonNativeOamPrepareCloudTextureBytes, gNdsIFCommonNativeOamPrepareCloudTextureCount, gNdsIFCommonNativeOamPreparePaletteBytes, gNdsIFCommonNativeOamPrepareBytes, gNdsIFCommonNativeOamHotConvertCount, gNdsIFCommonNativeOamRuntimeUploadBytes, gNdsRendererBattleTextureFenceCounts[0], gNdsRendererBattleTextureFenceCounts[1], gNdsRendererBattleTextureFenceCounts[2], gNdsRendererBattleTextureFenceCounts[3], gNdsRendererBattleTextureFenceCounts[4], gNdsRendererBattleTextureFenceCounts[5], gNdsRendererBattleTextureFenceCounts[6], gNdsRendererBattleTextureFenceCounts[7], gNdsRendererBattleTextureFenceCounts[8], gNdsRendererBattleTextureFenceCounts[9], gNdsRendererBattleTextureFenceFirstClassPlus1, gNdsRendererBattleTextureFenceFirstFrame, gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[0], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[1], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[2], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[3], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[4], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[5], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[6], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[7], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[8], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[9], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[10], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[11], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[12], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[13], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[14], gNdsIFCommonNativeOamPrepareCloudNonzeroTexels[15]'
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 10',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        "set variable gNdsIFCommonNativeOamEnabled = $NativeEnabled",
        'set variable gNdsIFCommonHUDLowerTextMode = 1',
        'break ndsBattlePlayableFrameCompleteMarker if ((gNdsRendererProfileFrameCount >= 186 && gNdsRendererProfileFrameCount <= 194) || (gNdsRendererProfileFrameCount >= 198 && gNdsRendererProfileFrameCount <= 202) || (gNdsRendererProfileFrameCount >= 250 && gNdsRendererProfileFrameCount <= 300) || gNdsRendererProfileFrameCount == 600)',
        'commands',
        'silent',
        'if gNdsRendererProfileFrameCount == 186',
        'printf "IFCOMMON_BASE=%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u\n", gNdsIFCommonNativeOamEnabled, gNdsRendererProfileFrameCount, gNdsIFCommonNativeOamCommitCount, gNdsOriginalSpriteBg3ClearBytes, gNdsOriginalSpriteBg3CopyBytes, gNdsOriginalSpriteBg3FinalWriteBytes, gNdsSObjForegroundStagingClearBytes, gNdsIFCommonNativeOamFrameObjectCount, gNdsIFCommonNativeOamFrameSemanticHash, gNdsIFCommonHUDTopGenericPassCount, gNdsIFCommonHUDLowerTimerRouteCount',
        'end',
        'if gNdsRendererProfileFrameCount >= 187 && gNdsRendererProfileFrameCount <= 194',
        ("printf `"$activeFormat\n`", $activeValues"),
        'end',
        'if gNdsRendererProfileFrameCount >= 198 && gNdsRendererProfileFrameCount <= 202',
        ("printf `"$goFormat\n`", $goValues"),
        'if gNdsRendererProfileFrameCount == 198',
        ("shell powershell.exe -NoProfile -Command `"Set-Content -LiteralPath '$gdbReady' -Value ready; while (-not (Test-Path -LiteralPath '$gdbGo')) { Start-Sleep -Milliseconds 25 }`""),
        'end',
        'end',
        'if gNdsRendererProfileFrameCount >= 250 && gNdsRendererProfileFrameCount <= 300',
        ("printf `"$tailFormat\n`", $tailValues"),
        'if gNdsRendererProfileFrameCount == 256',
        ("shell powershell.exe -NoProfile -Command `"Set-Content -LiteralPath '$gdbClearReady' -Value ready; while (-not (Test-Path -LiteralPath '$gdbClearGo')) { Start-Sleep -Milliseconds 25 }`""),
        'end',
        'end',
        'if gNdsRendererProfileFrameCount == 600',
        ("printf `"IFCOMMON_IDLE=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u\n`", $tailValues"),
        'disable 2',
        'end',
        'if gNdsRendererProfileFrameCount < 600',
        'continue',
        'end',
        'end',
        'continue',
        'printf "IFCOMMON_PREP=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", gNdsIFCommonNativeOamEnabled, gNdsIFCommonNativeOamPrepareCount, gNdsIFCommonNativeOamPrepareSuccessCount, gNdsIFCommonNativeOamPrepareFailCount, gNdsIFCommonNativeOamPrepareTicks, gNdsIFCommonNativeOamPrepareBytes, gNdsIFCommonNativeOamPrepareAssets, gNdsIFCommonNativeOamPrepareTiles, gNdsIFCommonNativeOamPrepareProfileFrame, gNdsIFCommonNativeOamPreparePaletteBytes, gNdsIFCommonNativeOamHotConvertCount, gNdsIFCommonNativeOamRuntimeUploadBytes',
        'detach',
        'quit'
    )
    Set-Content -LiteralPath $gdbScript -Value ($commands -join "`n")

    $emulator = $null
    $gdbProcess = $null
    try {
        $emulator = Start-Process -FilePath $context.MelonDSPath `
            -ArgumentList "`"$rom`"" `
            -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
            -RedirectStandardOutput $melonStdout `
            -RedirectStandardError $melonStderr -PassThru
        Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort |
            Out-Null
        $windowDeadline = (Get-Date).AddSeconds(15)
        do {
            Start-Sleep -Milliseconds 100
            $emulator.Refresh()
        } while ($emulator.MainWindowHandle -eq [IntPtr]::Zero -and
                 -not $emulator.HasExited -and (Get-Date) -lt $windowDeadline)
        Assert-Condition (-not $emulator.HasExited -and
            $emulator.MainWindowHandle -ne [IntPtr]::Zero) `
            'melonDS did not expose a capturable window.'
        # OpenGL surfaces do not repaint while the ARM9 is paused in GDB.
        # Establish stable visible geometry while emulation is still running;
        # the exact-frame capture below must not move or foreground the window.
        Set-WindowCapturePosition -WindowHandle $emulator.MainWindowHandle

        $gdbProcess = Start-Process -FilePath $Gdb `
            -ArgumentList @('-batch', '-x', $gdbScript, $elf) `
            -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
            -WindowStyle Hidden -PassThru
        $captureDeadline = (Get-Date).AddSeconds(120)
        while (-not (Test-Path -LiteralPath $ready)) {
            $gdbProcess.Refresh()
            if ($gdbProcess.HasExited) {
                $contextText = (Get-Content $stdout, $stderr -Raw `
                    -ErrorAction SilentlyContinue) -join "`n"
                throw "GDB exited before exact frame 198 capture.`n$contextText"
            }
            if ((Get-Date) -ge $captureDeadline) {
                throw 'Timed out waiting for exact frame 198 capture pause.'
            }
            Start-Sleep -Milliseconds 50
        }
        Save-WindowCapture -WindowHandle $emulator.MainWindowHandle `
            -Path $CapturePath
        Set-Content -LiteralPath $go -Value go

        $clearDeadline = (Get-Date).AddSeconds(120)
        while (-not (Test-Path -LiteralPath $clearReady)) {
            $gdbProcess.Refresh()
            if ($gdbProcess.HasExited) {
                $contextText = (Get-Content $stdout, $stderr -Raw `
                    -ErrorAction SilentlyContinue) -join "`n"
                throw "GDB exited before exact frame 256 capture.`n$contextText"
            }
            if ((Get-Date) -ge $clearDeadline) {
                throw 'Timed out waiting for exact frame 256 capture pause.'
            }
            Start-Sleep -Milliseconds 50
        }
        Save-WindowCapture -WindowHandle $emulator.MainWindowHandle `
            -Path $ClearCapturePath
        Set-Content -LiteralPath $clearGo -Value go

        Assert-Condition ($gdbProcess.WaitForExit(180000)) `
            'Timed out waiting for the exact native-OAM GDB run.'
        $gdbProcess.WaitForExit()
        $gdbText = ((Get-Content $stdout, $stderr -Raw `
            -ErrorAction SilentlyContinue) -join "`n").Trim()
        Assert-Condition ($gdbProcess.ExitCode -eq 0) `
            "GDB native-OAM run failed with exit $($gdbProcess.ExitCode)." `
            $gdbText
        $baseRows = @(Get-MarkerRows $gdbText 'IFCOMMON_BASE')
        $activeRows = @(Get-MarkerRows $gdbText 'IFCOMMON_ACTIVE')
        $tailRows = @(Get-MarkerRows $gdbText 'IFCOMMON_TAIL')
        $idleRows = @(Get-MarkerRows $gdbText 'IFCOMMON_IDLE')
        $prepRows = @(Get-MarkerRows $gdbText 'IFCOMMON_PREP')
        $goRows = @(Get-MarkerRows $gdbText 'IFCOMMON_GO')
        return [PSCustomObject]@{
            Mode = $NativeEnabled
            Text = $gdbText
            Base = if ($baseRows.Count -ne 0) { $baseRows[0] } else { $null }
            Active = $activeRows
            Tail = $tailRows
            Idle = if ($idleRows.Count -ne 0) { $idleRows[0] } else { $null }
            Prep = if ($prepRows.Count -ne 0) { $prepRows[0] } else { $null }
            Go = $goRows
            Capture = $CapturePath
            ClearCapture = $ClearCapturePath
            ConfigHash = $runConfigHash
        }
    } finally {
        if ($null -ne $gdbProcess) {
            $gdbProcess.Refresh()
            if (-not $gdbProcess.HasExited) {
                Stop-Process -Id $gdbProcess.Id -Force
                $gdbProcess.WaitForExit()
            }
        }
        if ($null -ne $emulator) {
            $emulator.Refresh()
            if (-not $emulator.HasExited) {
                Stop-Process -Id $emulator.Id -Force
                $emulator.WaitForExit()
            }
        }
        Remove-Item $ready, $go, $clearReady, $clearGo -Force `
            -ErrorAction SilentlyContinue
    }
}

$hostFixture = & python (Join-Path $root 'scripts\check_ifcommon_hybrid_oam.py') 2>&1
Assert-Condition ($LASTEXITCODE -eq 0) `
    'Offline IFCommon atlas/filter fixture failed before emulator execution.' `
    ($hostFixture -join "`n")

if (-not $NoBuild) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    & make -C $root "TARGET=$Target" "BUILD=$Build" `
        "NDS_IFCOMMON_HYBRID_OAM=$HybridOamMode" -j24
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
Assert-Condition (Test-Path -LiteralPath $rom) "ROM not found: $rom"
Assert-Condition (Test-Path -LiteralPath $elf) "ELF not found: $elf"
Assert-Condition (Test-Path -LiteralPath $Gdb) "GDB not found: $Gdb"
$buildPath = Resolve-Smash64DSBuildPath -Root $root -Build $Build
$configHeader = Join-Path $buildPath 'nds_build_config.h'
Assert-Condition (Test-Path -LiteralPath $configHeader) `
    "Build configuration not found: $configHeader"
$configHeaderText = Get-Content -LiteralPath $configHeader -Raw
Assert-Condition ($configHeaderText -match
    ('(?m)^#define NDS_IFCOMMON_HYBRID_OAM ' + $HybridOamMode + '$')) `
    "Compiled hybrid-OAM mode does not match requested mode $HybridOamMode." `
    $configHeaderText
New-Item -ItemType Directory -Force -Path $visibilityDir, $logDir, $tempDir |
    Out-Null

$configState = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath `
    -GdbPort $context.GdbPort -Arm7Port $context.Arm7Port `
    -Persistent:($RunnerSlot -ge 0) -MuteAudio
$configText = Get-Content -LiteralPath $configState.Config -Raw
$configText = Set-MelonDSDualScreenLayout -Text $configText
Set-Content -LiteralPath $configState.Config -Value $configText -NoNewline
$arm9Port = Get-TomlSetting $configText 'Instance0.Gdb.ARM9' 'Port'
$arm7Port = Get-TomlSetting $configText 'Instance0.Gdb.ARM7' 'Port'
$volume = Get-TomlSetting $configText 'Instance0.Audio' 'Volume'
Assert-Condition ($arm9Port -eq '4463' -and $arm7Port -eq '4464' -and
    $volume -eq '0') `
    "Runner slot 2 config is not host-muted on dedicated ports: ARM9=$arm9Port ARM7=$arm7Port Volume=$volume."
$configHash = (Get-FileHash -LiteralPath $configState.Config -Algorithm SHA256).Hash
$romHash = (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash
$elfHash = (Get-FileHash -LiteralPath $elf -Algorithm SHA256).Hash
$date = Get-Date -Format 'yyyy-MM-dd'
$captureVariant = if ($HybridOamMode -eq 1) { '-hybrid' } else { '' }
$fallbackCapture = Join-Path $visibilityDir `
    "task11-${date}_ifcommon-bg3-fallback${captureVariant}-frame198.png"
$nativeCapture = Join-Path $visibilityDir `
    "task11-${date}_ifcommon-native-oam${captureVariant}-frame198.png"
$fallbackClearCapture = Join-Path $visibilityDir `
    "task11-${date}_ifcommon-bg3-fallback${captureVariant}-frame256.png"
$nativeClearCapture = Join-Path $visibilityDir `
    "task11-${date}_ifcommon-native-oam${captureVariant}-frame256.png"

try {
    # GO occupies three direct RGB555+A1 OBJ cells. Traffic stays one opaque
    # A3I5 hardware atlas and the flare stays two source-alpha A5I3 atlases.
    # Palette accounting is 512 OBJ + 2*16 A5I3 + 64 A3I5 = 608 bytes.
    $expectedPrepareBytes = 31168
    $expectedPaletteBytes = 608
    $expectedCloudTextureBytes = 57344
    $expectedCloudTextureCount = 3
    $expectedCloudNonzero = @(
        486, 421, 739, 6001, 7067, 8990, 0, 0, 0,
        176, 2026, 97, 70, 132, 75, 201)
    Assert-RunnerReleased
    Set-Content -LiteralPath $configState.Config -Value $configText -NoNewline
    $fallback = Invoke-IFCommonRun -NativeEnabled 0 `
        -CapturePath $fallbackCapture -ClearCapturePath $fallbackClearCapture
    Assert-RunnerReleased
    Set-Content -LiteralPath $configState.Config -Value $configText -NoNewline
    $native = Invoke-IFCommonRun -NativeEnabled 1 `
        -CapturePath $nativeCapture -ClearCapturePath $nativeClearCapture
    Assert-RunnerReleased
    Set-Content -LiteralPath $configState.Config -Value $configText -NoNewline

    # Canonical window coordinates derived from the accepted frame-198 capture.
    # The exact GO crop locks the owner's approved nearest-pixel presentation; the
    # traffic crop excludes the unrelated lower-stage geometry and separately
    # proves a blue flare, shaded housing/lamps, and countdown disappearance.
    $trafficRegion = New-Object System.Drawing.Rectangle 115, 56, 235, 120
    $goRegion = New-Object System.Drawing.Rectangle 100, 155, 220, 120
    $trafficMetrics = Get-BitmapRegionMetrics -Path $nativeCapture `
        -ClearPath $nativeClearCapture -Region $trafficRegion
    $goMetrics = Get-BitmapRegionMetrics -Path $nativeCapture `
        -ClearPath $nativeClearCapture -Region $goRegion
    Assert-Condition ($trafficMetrics.Hash -eq
        '8971bfb5863c6e3ec55e4bda8049545974f881d11af29f69e74b982a865589ac') `
        'Accepted traffic-light/flare crop changed byte-for-byte.'
    Assert-Condition ($goMetrics.Hash -eq
        'd968b0cc6513a5736200247d46dedee9eb2bb88919420931cfd9c81cde0a4619') `
        'Accepted GO text crop changed byte-for-byte.'
    Assert-Condition ($trafficMetrics.Blue -gt 10000 -and
        $trafficMetrics.Red -gt 800 -and $trafficMetrics.Amber -gt 800 -and
        $trafficMetrics.Dark -gt 500 -and
        $trafficMetrics.Different -gt 25000) `
        'Traffic-light crop lost the blue flare, shaded lamps/housing, or disappearance.'
    Assert-Condition ($goMetrics.Red -gt 6500 -and
        $goMetrics.Amber -gt 4000 -and $goMetrics.Cream -gt 3500 -and
        $goMetrics.Different -gt 20000) `
        'GO crop lost its red/amber/cream body or countdown disappearance.'
    $fallbackGoCrop = Join-Path $visibilityDir `
        "task11-${date}_ifcommon-go-fallback${captureVariant}-frame198-3x.png"
    $acceptedGoCrop = Join-Path $visibilityDir `
        "task11-${date}_ifcommon-go-accepted${captureVariant}-frame198-3x.png"
    Save-NearestNeighborCrop -SourcePath $fallbackCapture `
        -DestinationPath $fallbackGoCrop -Region $goRegion
    Save-NearestNeighborCrop -SourcePath $nativeCapture `
        -DestinationPath $acceptedGoCrop -Region $goRegion

    foreach ($run in @($fallback, $native)) {
        Assert-Condition ($null -ne $run.Base -and $null -ne $run.Prep -and
            $null -ne $run.Idle) `
            "Mode $($run.Mode) missed base, preparation, or idle markers." `
            $run.Text
        Assert-Condition ($run.Active.Count -eq 8) `
            "Mode $($run.Mode) captured $($run.Active.Count) of 8 active frames." `
            $run.Text
        Assert-Condition ($run.Go.Count -eq 5) `
            "Mode $($run.Mode) captured $($run.Go.Count) of 5 GO frames." `
            $run.Text
        Assert-Condition ($run.Tail.Count -eq 51) `
            "Mode $($run.Mode) captured $($run.Tail.Count) of 51 tail frames." `
            $run.Text
        $expectedFrames = @(187..194)
        $actualFrames = @($run.Active | ForEach-Object { [int64]$_[1] })
        Assert-Condition (($actualFrames -join ',') -eq
            ($expectedFrames -join ',')) `
            "Mode $($run.Mode) did not capture exact frames 187..194." `
            $run.Text
        $expectedGoFrames = @(198..202)
        $actualGoFrames = @($run.Go | ForEach-Object { [int64]$_[0] })
        Assert-Condition (($actualGoFrames -join ',') -eq
            ($expectedGoFrames -join ',')) `
            "Mode $($run.Mode) did not capture exact GO frames 198..202." `
            $run.Text
        $prep = $run.Prep
        Assert-Condition ($prep[0] -eq $run.Mode -and $prep[1] -eq 1 -and
            $prep[2] -eq 1 -and $prep[3] -eq 0 -and $prep[4] -gt 0 -and
            $prep[5] -eq $expectedPrepareBytes -and $prep[6] -eq 16 -and
            $prep[7] -eq 25 -and $prep[8] -eq 0 -and
            $prep[9] -eq $expectedPaletteBytes -and
            $prep[10] -eq 0 -and $prep[11] -eq 0) `
            'Native countdown assets were not prepared exactly once outside gameplay with the derived palette cost and no hot/runtime upload.' `
            $run.Text
        foreach ($row in $run.Active) {
            Assert-Condition ($row[0] -eq $run.Mode -and $row[9] -eq 1 -and
                $row[12] -eq 10 -and $row[32] -eq 0 -and
                $row[33] -eq 0 -and
                $row[34] -eq $expectedPaletteBytes -and
                $row[35] -eq 1 -and $row[36] -eq 1) `
                "Mode $($run.Mode) lost source recognition or performed hot conversion/upload." `
                $run.Text
            Assert-Condition ($row[23] -eq 0 -and $row[24] -eq 0 -and
                $row[25] -eq 3600 -and $row[26] -eq 0 -and
                $row[27] -eq 1 -and $row[28] -eq 1) `
                "Mode $($run.Mode) disagreed with BattleShip's locked Wait/timer state." `
                $run.Text
        }
        foreach ($row in $run.Go) {
            Assert-Condition ($row.Count -eq 48 -and
                $row[1] -eq 1 -and $row[5] -eq 0 -and
                $row[6] -ne 0 -and $row[7] -ne 0 -and
                $row[8] -ne 0 -and
                $row[14] -eq $expectedCloudTextureBytes -and
                $row[15] -eq $expectedCloudTextureCount -and
                $row[16] -eq $expectedPaletteBytes -and
                $row[17] -eq $expectedPrepareBytes -and
                $row[18] -eq 0 -and $row[19] -eq 0 -and
                (($row[20..29] | Measure-Object -Sum).Sum -eq 0) -and
                $row[30] -eq 0 -and $row[31] -eq 0 -and
                (($row[32..47] -join ',') -eq
                    ($expectedCloudNonzero -join ','))) `
                "Mode $($run.Mode) lost prepared overlay residency, direct GO cells, or the post-GO texture fence." `
                $run.Text
        }
    }

    foreach ($row in $fallback.Go) {
        Assert-Condition ($row[2] -eq 0 -and $row[3] -eq 0 -and
            $row[4] -eq 0 -and $row[10] -eq 0 -and $row[11] -eq 2) `
            'Disabled native mode unexpectedly queued an IFCommon overlay.' `
            $fallback.Text
    }
    foreach ($row in $native.Go) {
        Assert-Condition ($row[2] -eq 10 -and $row[3] -eq 10 -and
            $row[4] -eq 10 -and $row[9] -eq 0 -and
            $row[10] -eq 2 -and $row[11] -eq 0 -and
            $row[12] -eq 3 -and $row[13] -eq 13) `
            'Native GO did not emit three direct letters plus traffic and flare exactly once.' `
            $native.Text
    }

    for ($i = 0; $i -lt 8; $i++) {
        $a = $fallback.Active[$i]
        $b = $native.Active[$i]
        Assert-Condition ($a[1] -eq $b[1] -and $a[2] -eq $b[2] -and
            $a[12] -eq $b[12] -and $a[14] -eq $b[14] -and
            (($a[23..31] -join ',') -eq ($b[23..31] -join ','))) `
            "Fallback/native source semantics diverged at exact frame $($a[1])." `
            ($fallback.Text + "`n" + $native.Text)
    }

    foreach ($row in $fallback.Active) {
        Assert-Condition ($row[6] -eq 0 -and $row[7] -eq 0 -and
            $row[8] -eq 1 -and $row[10] -eq 0 -and $row[11] -eq 1 -and
            $row[13] -eq 0 -and $row[15] -eq 1) `
            'Disabled native mode did not take the exact generic BG3 fallback.' `
            $fallback.Text
    }
    $fallbackStaging = Get-Delta $fallback.Base $fallback.Active 22 6
    $fallbackBg3Clear = Get-Delta $fallback.Base $fallback.Active 19 3
    $fallbackBg3Copy = Get-Delta $fallback.Base $fallback.Active 20 4
    $fallbackBg3Final = Get-Delta $fallback.Base $fallback.Active 21 5
    Assert-Condition (@($fallbackStaging | Where-Object { $_ -ne 153600 }).Count -eq 0 -and
        @($fallbackBg3Clear | Where-Object { $_ -ne 0 }).Count -eq 0 -and
        @($fallbackBg3Copy | Where-Object { $_ -ne 98304 }).Count -eq 0 -and
        @($fallbackBg3Final | Where-Object { $_ -ne 0 }).Count -eq 0) `
        'Generic A/B owner did not perform exactly one foreground staging clear and BG3 copy per active frame.' `
        $fallback.Text

    $nativeInclusive = @()
    $previousCommit = [int64]$native.Base[2]
    foreach ($row in $native.Active) {
        $nativeInclusive += [int64]$row[3] + [int64]$row[4] + [int64]$row[5]
        Assert-Condition ($row[6] -eq 0 -and $row[7] -eq 0 -and
            $row[8] -eq 1 -and $row[10] -eq 1 -and $row[11] -eq 0 -and
            $row[13] -eq 0 -and $row[15] -eq 0 -and
            $row[16] -eq $previousCommit -and $row[17] -ge $row[5]) `
            'Native quad frame retained an OAM clear/object/commit or lost its source draw.' `
            $native.Text
        $previousCommit = [int64]$row[16]
    }
    $nativeMedian = Get-Median $nativeInclusive
    $nativeP95 = Get-Percentile95 $nativeInclusive
    Assert-Condition ($nativeMedian -ge 5000 -and $nativeMedian -le 35000 -and
        $nativeP95 -ge 5000 -and $nativeP95 -le 35000) `
        "Inclusive native OAM cost missed the 5K..35K gate: $nativeMedian/$nativeP95." `
        $native.Text
    foreach ($index in 19..22) {
        $baselineIndex = $index - 16
        $deltas = Get-Delta $native.Base $native.Active $index $baselineIndex
        Assert-Condition (@($deltas | Where-Object { $_ -ne 0 }).Count -eq 0) `
            "Native OAM active frames retained generic BG3/staging owner $index." `
            $native.Text
    }
    $fallbackForeground = @($fallback.Active | ForEach-Object { [int64]$_[18] })
    $nativeForeground = @($native.Active | ForEach-Object { [int64]$_[18] })
    Assert-Condition ((Get-Median $fallbackForeground) -
        (Get-Median $nativeForeground) -ge 1500000) `
        'Native OAM did not remove at least 1.5M median foreground ticks from the identical window.'

    $nativeTail = $native.Tail
    $firstZero = -1
    for ($i = 1; $i -lt $nativeTail.Count; $i++) {
        if ($nativeTail[$i - 1][11] -gt 0 -and $nativeTail[$i][11] -eq 0) {
            $firstZero = $i
            break
        }
    }
    Assert-Condition ($firstZero -gt 0) `
        'Tail window did not observe the countdown final-clear transition.' `
        $native.Text
    $lastActive = $nativeTail[$firstZero - 1]
    $finalClear = $nativeTail[$firstZero]
    Assert-Condition ($lastActive[1] -eq 255 -and
        $finalClear[1] -eq 256 -and
        $finalClear[2] -eq 0 -and $finalClear[3] -eq 0 -and
        $finalClear[4] -eq 0 -and $finalClear[5] -eq 0 -and
        $finalClear[6] -eq 0 -and $finalClear[7] -eq 1 -and
        $finalClear[8] -eq 0 -and $finalClear[9] -eq 0 -and
        $finalClear[11] -eq 0 -and $finalClear[12] -eq 0 -and
        $finalClear[15] -eq $lastActive[15] -and
        $finalClear[22] -ge $finalClear[4]) `
        'Countdown disappearance retained native OAM work after the quad queue emptied.' `
        $native.Text
    for ($i = $firstZero + 1; $i -lt $nativeTail.Count; $i++) {
        $row = $nativeTail[$i]
        Assert-Condition ($row[2] -eq 0 -and $row[3] -eq 0 -and
            $row[4] -eq 0 -and $row[5] -eq 0 -and $row[6] -eq 0 -and
            $row[7] -eq 1 -and $row[8] -eq 0 -and $row[9] -eq 0 -and
            $row[10] -eq 0 -and $row[11] -eq 0 -and $row[12] -eq 0 -and
            $row[15] -eq $finalClear[15]) `
            'Post-countdown idle frame retained an OAM clear, scan, draw, or commit tax.' `
            $native.Text
    }
    foreach ($row in $nativeTail) {
        Assert-Condition ($row[16] -eq 1 -and $row[17] -eq 1 -and
            ($row[18] + $row[19]) -eq 3600 -and $row[19] -gt 0 -and
            $row[20] -eq 0 -and $row[21] -eq 0) `
            'Tail window disagreed with BattleShip GO, running timer, or unlocked controls.' `
            $native.Text
    }
    $idle = $native.Idle
    Assert-Condition ($idle[1] -eq 600 -and $idle[2] -eq 0 -and
        $idle[3] -eq 0 -and $idle[4] -eq 0 -and $idle[5] -eq 0 -and
        $idle[6] -eq 0 -and $idle[7] -eq 1 -and $idle[8] -eq 0 -and
        $idle[9] -eq 0 -and $idle[10] -eq 0 -and $idle[11] -eq 0 -and
        $idle[12] -eq 0 -and $idle[15] -eq $finalClear[15]) `
        'Frame 600 retained a permanent native countdown OAM tax.' `
        $native.Text

    Assert-Condition ($fallback.ConfigHash -eq $configHash -and
        $native.ConfigHash -eq $configHash -and
        (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash -eq $romHash -and
        (Get-FileHash -LiteralPath $elf -Algorithm SHA256).Hash -eq $elfHash -and
        (Get-FileHash -LiteralPath $configState.Config -Algorithm SHA256).Hash -eq
        $configHash) `
        'A/B runs did not retain the identical ROM, ELF, and melonDS configuration.'

    Write-Output (
        "IFCommon native OAM gate passed: ROM=$romHash ELF=$elfHash " +
        "frames=187..194 sourceSObjs=10 objects=0 GO=3obj+10quads " +
        "hybridOam=$HybridOamMode prepareBytes=$expectedPrepareBytes paletteBytes=$expectedPaletteBytes " +
        "inclusiveMedian/P95=$nativeMedian/$nativeP95 " +
        "foregroundMedian=$(Get-Median $fallbackForeground)->$(Get-Median $nativeForeground) " +
        "finalClearFrame=$($finalClear[1]) idleFrame=$($idle[1]) " +
        "config=$configHash captures='$fallbackCapture','$nativeCapture'")
} finally {
    Restore-MelonDSGdbConfig -State $configState
}
