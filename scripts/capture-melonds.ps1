param(
    [switch]$Build,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Rom = (Join-Path $PSScriptRoot '..\smash64ds.nds'),
    [string]$Output,
    [int]$DelaySeconds = 32,
    [switch]$Unthrottled,
    [switch]$OpenGL4x,
    [switch]$SoftwareRenderer,
    [switch]$MaximizeVertical,
    [string]$SecondOutput,
    [int]$SecondDelaySeconds = 1,
    [int]$SecondDelayMilliseconds = 0
)
$ErrorActionPreference = 'Stop'
if ($OpenGL4x -and $SoftwareRenderer) {
    throw '-OpenGL4x and -SoftwareRenderer are mutually exclusive.'
}
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$melonDsPath = if ([System.IO.Path]::IsPathRooted($MelonDS)) {
    $MelonDS
} else {
    Join-Path $root $MelonDS
}
$melonDsDir = Split-Path -Parent $melonDsPath
$romPath = if ([System.IO.Path]::IsPathRooted($Rom)) {
    $Rom
} else {
    Join-Path $root $Rom
}
$config = Join-Path $melonDsDir 'melonDS.toml'
$originalConfig = $null
if ($Build) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    & make -C $root -j16
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if (-not (Test-Path $melonDsPath)) {
    throw "melonDS executable not found: $melonDsPath"
}
if (-not (Test-Path $romPath)) {
    throw "ROM not found: $romPath. Run make first or pass -Build."
}
if ([string]::IsNullOrWhiteSpace($Output)) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $Output = Join-Path $root "artifacts\melonds-$stamp.png"
} elseif (-not [System.IO.Path]::IsPathRooted($Output)) {
    $Output = Join-Path $root $Output
}
$outputDirectory = Split-Path -Parent $Output
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
if (-not [string]::IsNullOrWhiteSpace($SecondOutput)) {
    if (-not [System.IO.Path]::IsPathRooted($SecondOutput)) {
        $SecondOutput = Join-Path $root $SecondOutput
    }
    $secondOutputDirectory = Split-Path -Parent $SecondOutput
    New-Item -ItemType Directory -Path $secondOutputDirectory -Force | Out-Null
}
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Smash64DSWindowCapture
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
function Save-MelonDSWindowCapture {
    param(
        [Parameter(Mandatory=$true)]
        [System.IntPtr]$WindowHandle,
        [Parameter(Mandatory=$true)]
        [string]$Path
    )
    $rect = New-Object Smash64DSWindowCapture+Rect
    if (-not [Smash64DSWindowCapture]::GetWindowRect(
            $WindowHandle, [ref]$rect)) {
        throw 'Could not read the melonDS window bounds.'
    }
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -le 0 -or $height -le 0) {
        throw "Invalid melonDS window bounds: ${width}x${height}."
    }
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
    return "${width}x${height}"
}
function Set-MelonDSCaptureWindow {
    param(
        [Parameter(Mandatory=$true)]
        [System.IntPtr]$WindowHandle
    )
    if ($MaximizeVertical) {
        # Preserve the verifier window's aspect ratio while using the desktop's
        # available vertical resolution for close visual inspection.
        $workArea = [System.Windows.Forms.Screen]::PrimaryScreen.WorkingArea
        $height = $workArea.Height
        $width = [Math]::Round($height * (488.0 / 675.0))
        [void][Smash64DSWindowCapture]::ShowWindow($WindowHandle, 9)
        [void][Smash64DSWindowCapture]::SetWindowPos(
            $WindowHandle, [IntPtr](-1), $workArea.X, $workArea.Y,
            $width, $height, 0x40)
        return
    }
    [void][Smash64DSWindowCapture]::ShowWindow($WindowHandle, 9)
    # Keep capture geometry stable between samples. melonDS can otherwise
    # auto-resize after the first frame and create a false pixel-delta failure.
    # SWP_NOMOVE | SWP_SHOWWINDOW = 0x42.
    [void][Smash64DSWindowCapture]::SetWindowPos(
        $WindowHandle, [IntPtr](-1), 0, 0, 488, 675, 0x42)
}
$emulator = $null
try {
    if (Test-Path $config) {
        $originalConfig = Get-Content $config -Raw
        $visibleConfig = $originalConfig -replace
            '(?s)(\[Instance0\.Gdb\]\s*Enabled\s*=\s*)true', '${1}false'
        $visibleConfig = $visibleConfig -replace
            '(?s)(\[Instance0\.Gdb\]\s*Enable\s*=\s*)true', '${1}false'
        if ($Unthrottled) {
            $visibleConfig = $visibleConfig -replace
                '(?m)^(LimitFPS\s*=\s*)true\s*$', '${1}false'
            $visibleConfig = $visibleConfig -replace
                '(?ms)(\[JIT\].*?^Enable\s*=\s*)true\s*$', '${1}false'
        }
        if ($SoftwareRenderer) {
            $visibleConfig = $visibleConfig -replace
                '(?m)^(Renderer\s*=\s*)[0-9]+\s*$', '${1}0'
        } elseif ($OpenGL4x) {
            $visibleConfig = $visibleConfig -replace
                '(?m)^(Renderer\s*=\s*)[0-9]+\s*$', '${1}1'
            $visibleConfig = $visibleConfig -replace
                '(?m)^(ScaleFactor\s*=\s*)[0-9]+\s*$', '${1}4'
        }
        if ($visibleConfig -ne $originalConfig) {
            Set-Content $config -Value $visibleConfig -NoNewline
        }
    }
    $emulator = Start-Process -FilePath $melonDsPath -ArgumentList "`"$romPath`"" `
        -WorkingDirectory $melonDsDir -PassThru
    $deadline = (Get-Date).AddSeconds([Math]::Max($DelaySeconds, 2) + 10)
    do {
        Start-Sleep -Milliseconds 250
        $emulator.Refresh()
    } while ($emulator.MainWindowHandle -eq [IntPtr]::Zero -and
             -not $emulator.HasExited -and (Get-Date) -lt $deadline)
    if ($emulator.HasExited -or $emulator.MainWindowHandle -eq [IntPtr]::Zero) {
        throw 'melonDS did not expose a capturable window.'
    }
    Set-MelonDSCaptureWindow -WindowHandle $emulator.MainWindowHandle
    [void][Smash64DSWindowCapture]::SetForegroundWindow($emulator.MainWindowHandle)
    Start-Sleep -Seconds $DelaySeconds
    $emulator.Refresh()
    Set-MelonDSCaptureWindow -WindowHandle $emulator.MainWindowHandle
    Start-Sleep -Milliseconds 100
    $size = Save-MelonDSWindowCapture -WindowHandle $emulator.MainWindowHandle `
        -Path $Output
    if (-not [string]::IsNullOrWhiteSpace($SecondOutput)) {
        if ($SecondDelayMilliseconds -gt 0) {
            Start-Sleep -Milliseconds $SecondDelayMilliseconds
        } else {
            Start-Sleep -Seconds ([Math]::Max($SecondDelaySeconds, 0))
        }
        $emulator.Refresh()
        Set-MelonDSCaptureWindow -WindowHandle $emulator.MainWindowHandle
        Start-Sleep -Milliseconds 100
        $secondSize = Save-MelonDSWindowCapture `
            -WindowHandle $emulator.MainWindowHandle -Path $SecondOutput
        Write-Output "Captured live melonDS window: $SecondOutput ($secondSize)"
    }
    try {
        [void][Smash64DSWindowCapture]::SetWindowPos(
            $emulator.MainWindowHandle, [IntPtr](-2), 0, 0, 0, 0, 0x43)
    } catch {
        Write-Warning "Could not lower melonDS window: $_"
    }
    Write-Output "Captured live melonDS window: $Output ($size)"
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    if ($null -ne $originalConfig) {
        Set-Content $config -Value $originalConfig -NoNewline
    }
}
