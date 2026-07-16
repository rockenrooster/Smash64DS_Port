[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [int]$EmulatorProcessId,
    [Parameter(Mandatory=$true)]
    [string]$Output
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$visibilityDir = [System.IO.Path]::GetFullPath(
    (Join-Path $root 'artifacts\visibility'))
$outputPath = if ([System.IO.Path]::IsPathRooted($Output)) {
    [System.IO.Path]::GetFullPath($Output)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $Output))
}
$visibilityPrefix = $visibilityDir.TrimEnd('\', '/') +
    [System.IO.Path]::DirectorySeparatorChar
if (-not $outputPath.StartsWith(
        $visibilityPrefix,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "melonDS captures must stay under '$visibilityDir': $outputPath"
}

$emulatorsDir = [System.IO.Path]::GetFullPath(
    (Join-Path $root 'emulators')).TrimEnd('\', '/') +
    [System.IO.Path]::DirectorySeparatorChar
$process = Get-Process -Id $EmulatorProcessId -ErrorAction Stop
$processPath = [System.IO.Path]::GetFullPath($process.Path)
if (-not $processPath.StartsWith(
        $emulatorsDir,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to capture non-repo melonDS process $EmulatorProcessId at '$processPath'."
}

Add-Type -AssemblyName System.Drawing
if ($null -eq ('Smash64DSRunningMelonDSCapture' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Smash64DSRunningMelonDSCapture
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
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr window, IntPtr insertAfter,
        int x, int y, int width, int height, uint flags);
    private delegate bool EnumWindowsProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")]
    private static extern bool EnumWindows(
        EnumWindowsProc callback, IntPtr parameter);
    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(
        IntPtr window, out uint processId);

    public static IntPtr FindTopLevelWindow(int processId)
    {
        IntPtr found = IntPtr.Zero;
        EnumWindows(delegate(IntPtr window, IntPtr parameter)
        {
            uint owner;
            GetWindowThreadProcessId(window, out owner);
            if (owner == (uint)processId)
            {
                found = window;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
"@
}

$deadline = (Get-Date).AddSeconds(10)
$handle = [IntPtr]::Zero
do {
    $process.Refresh()
    if ($process.HasExited) {
        throw 'melonDS exited before evidence capture.'
    }
    $handle = $process.MainWindowHandle
    if ($handle -eq [IntPtr]::Zero) {
        $handle = [Smash64DSRunningMelonDSCapture]::FindTopLevelWindow(
            $process.Id)
    }
    if ($handle -ne [IntPtr]::Zero) { break }
    Start-Sleep -Milliseconds 50
} while ((Get-Date) -lt $deadline)

if ($handle -eq [IntPtr]::Zero) {
    throw 'melonDS did not expose a window for evidence capture.'
}

# Match scripts/lib/melonds.ps1's canonical stacked-screen window profile.
[void][Smash64DSRunningMelonDSCapture]::ShowWindow($handle, 9)
[void][Smash64DSRunningMelonDSCapture]::SetWindowPos(
    $handle, [IntPtr](-1),
    $script:MelonDSCanonicalWindowX,
    $script:MelonDSCanonicalWindowY,
    $script:MelonDSCanonicalWindowWidth,
    $script:MelonDSCanonicalWindowHeight,
    0x40)
[void][Smash64DSRunningMelonDSCapture]::SetForegroundWindow($handle)
Start-Sleep -Milliseconds 250

$rect = New-Object Smash64DSRunningMelonDSCapture+Rect
if (-not [Smash64DSRunningMelonDSCapture]::GetWindowRect(
        $handle, [ref]$rect)) {
    throw 'Could not read the melonDS evidence window bounds.'
}
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
if (($width -le 0) -or ($height -le 0)) {
    throw "Invalid melonDS evidence window bounds ${width}x${height}."
}
if (($width -ne $script:MelonDSCanonicalWindowWidth) -or
    ($height -ne $script:MelonDSCanonicalWindowHeight)) {
    $message = (
        'melonDS evidence window did not use the canonical {0}x{1} bounds: ' +
        '{2}x{3}.') -f
        $script:MelonDSCanonicalWindowWidth,
        $script:MelonDSCanonicalWindowHeight,
        $width,
        $height
    throw $message
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outputPath) |
    Out-Null
$bitmap = New-Object System.Drawing.Bitmap $width, $height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
try {
    $graphics.CopyFromScreen(
        $rect.Left, $rect.Top, 0, 0, $bitmap.Size)
    $bitmap.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Png)
} finally {
    $graphics.Dispose()
    $bitmap.Dispose()
    [void][Smash64DSRunningMelonDSCapture]::SetWindowPos(
        $handle, [IntPtr](-2), 0, 0, 0, 0, 0x43)
}

Write-Output "Captured repo-local melonDS window: $outputPath"
