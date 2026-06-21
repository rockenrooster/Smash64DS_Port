param(
    [switch]$Build,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Rom = (Join-Path $PSScriptRoot '..\smash64ds.nds'),
    [string]$Output,
    [int]$DelaySeconds = 32
)

$ErrorActionPreference = 'Stop'
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
    & make -C $root -j4
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

Add-Type -AssemblyName System.Drawing
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

$emulator = $null
try {
    if (Test-Path $config) {
        $originalConfig = Get-Content $config -Raw
        $visibleConfig = $originalConfig -replace `
            '(?s)(\[Instance0\.Gdb\]\s*Enabled\s*=\s*)true', '${1}false'
        $visibleConfig = $visibleConfig -replace `
            '(?s)(\[Instance0\.Gdb\]\s*Enable\s*=\s*)true', '${1}false'
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

    [void][Smash64DSWindowCapture]::ShowWindow($emulator.MainWindowHandle, 9)
    [void][Smash64DSWindowCapture]::SetForegroundWindow($emulator.MainWindowHandle)
    # Keep the emulator above Codex/terminal activity while CopyFromScreen reads
    # its pixels. SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW = 0x43.
    [void][Smash64DSWindowCapture]::SetWindowPos(
        $emulator.MainWindowHandle, [IntPtr](-1), 0, 0, 0, 0, 0x43)
    Start-Sleep -Seconds $DelaySeconds
    $emulator.Refresh()

    $rect = New-Object Smash64DSWindowCapture+Rect
    if (-not [Smash64DSWindowCapture]::GetWindowRect(
            $emulator.MainWindowHandle, [ref]$rect)) {
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
        $bitmap.Save($Output, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
        [void][Smash64DSWindowCapture]::SetWindowPos(
            $emulator.MainWindowHandle, [IntPtr](-2), 0, 0, 0, 0, 0x43)
    }

    Write-Output "Captured live melonDS window: $Output (${width}x${height})"
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
