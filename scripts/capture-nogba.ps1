param(
    [switch]$Build,
    [string]$NoGba = (Join-Path $PSScriptRoot '..\emulators\nogba\NO$GBA.EXE'),
    [string]$Rom = (Join-Path $PSScriptRoot '..\smash64ds.nds'),
    [string]$Output,
    [int]$DelaySeconds = 5,
    [int]$WindowIndex = 0,
    [switch]$AllWindows,
    [string[]]$EmulatorArgs = @()
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$noGbaPath = if ([System.IO.Path]::IsPathRooted($NoGba)) {
    $NoGba
} else {
    Join-Path $root $NoGba
}
$noGbaDir = Split-Path -Parent $noGbaPath
$romPath = if ([System.IO.Path]::IsPathRooted($Rom)) {
    $Rom
} else {
    Join-Path $root $Rom
}

if ($Build) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    & make -C $root -j4
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (-not (Test-Path $noGbaPath)) {
    throw "no`$gba executable not found: $noGbaPath. Place NO`$GBA.EXE in emulators\nogba or pass -NoGba."
}
if (-not (Test-Path $romPath)) {
    throw "ROM not found: $romPath. Run `make -j4` first or pass -Build."
}

if ([string]::IsNullOrWhiteSpace($Output)) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $Output = Join-Path $root "artifacts\nogba-$stamp.png"
} elseif (-not [System.IO.Path]::IsPathRooted($Output)) {
    $Output = Join-Path $root $Output
}

$outputDirectory = Split-Path -Parent $Output
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class Smash64DSNoGbaWindowCapture
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    public sealed class WindowInfo
    {
        public IntPtr Handle;
        public string Title;
        public int Left;
        public int Top;
        public int Width;
        public int Height;
        public long Area;
    }

    private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    private static extern bool EnumWindows(EnumWindowsProc callback, IntPtr lParam);

    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out int processId);

    [DllImport("user32.dll")]
    private static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int maxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetWindowTextLength(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);

    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(
        IntPtr window, IntPtr insertAfter, int x, int y, int width, int height,
        uint flags);

    public static WindowInfo[] GetWindowsForProcess(int processId)
    {
        List<WindowInfo> windows = new List<WindowInfo>();

        EnumWindows(delegate(IntPtr hWnd, IntPtr lParam) {
            int ownerPid;
            Rect rect;
            int width;
            int height;

            GetWindowThreadProcessId(hWnd, out ownerPid);
            if (ownerPid != processId || !IsWindowVisible(hWnd)) {
                return true;
            }
            if (!GetWindowRect(hWnd, out rect)) {
                return true;
            }

            width = rect.Right - rect.Left;
            height = rect.Bottom - rect.Top;
            if (width <= 0 || height <= 0) {
                return true;
            }

            int textLength = GetWindowTextLength(hWnd);
            StringBuilder title = new StringBuilder(textLength + 1);
            if (textLength > 0) {
                GetWindowText(hWnd, title, title.Capacity);
            }

            windows.Add(new WindowInfo {
                Handle = hWnd,
                Title = title.ToString(),
                Left = rect.Left,
                Top = rect.Top,
                Width = width,
                Height = height,
                Area = (long)width * (long)height
            });
            return true;
        }, IntPtr.Zero);

        windows.Sort(delegate(WindowInfo a, WindowInfo b) {
            int areaCompare = b.Area.CompareTo(a.Area);
            if (areaCompare != 0) {
                return areaCompare;
            }
            return String.Compare(a.Title, b.Title, StringComparison.Ordinal);
        });
        return windows.ToArray();
    }
}
'@

function Get-WindowOutputPath {
    param(
        [string]$BaseOutput,
        [int]$Index,
        [bool]$Multiple
    )

    if (-not $Multiple) {
        return $BaseOutput
    }

    $directory = Split-Path -Parent $BaseOutput
    $name = [System.IO.Path]::GetFileNameWithoutExtension($BaseOutput)
    $extension = [System.IO.Path]::GetExtension($BaseOutput)
    if ([string]::IsNullOrWhiteSpace($extension)) {
        $extension = '.png'
    }
    return Join-Path $directory ("{0}-w{1:00}{2}" -f $name, $Index, $extension)
}

function Save-WindowCapture {
    param(
        $Window,
        [int]$Index,
        [string]$Path
    )

    [void][Smash64DSNoGbaWindowCapture]::ShowWindow($Window.Handle, 9)
    [void][Smash64DSNoGbaWindowCapture]::SetForegroundWindow($Window.Handle)
    [void][Smash64DSNoGbaWindowCapture]::SetWindowPos(
        $Window.Handle, [IntPtr](-1), 0, 0, 0, 0, 0x43)

    Start-Sleep -Milliseconds 250

    $rect = New-Object Smash64DSNoGbaWindowCapture+Rect
    if (-not [Smash64DSNoGbaWindowCapture]::GetWindowRect(
            $Window.Handle, [ref]$rect)) {
        throw 'Could not read the no$gba window bounds.'
    }

    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -le 0 -or $height -le 0) {
        throw "Invalid no`$gba window bounds: ${width}x${height}."
    }

    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
        [void][Smash64DSNoGbaWindowCapture]::SetWindowPos(
            $Window.Handle, [IntPtr](-2), 0, 0, 0, 0, 0x43)
    }

    Write-Output ("Captured no`$gba window {0}: {1} ({2}x{3}) title='{4}'" -f `
        $Index, $Path, $width, $height, $Window.Title)
}

$emulator = $null
try {
    $argsList = @("`"$romPath`"") + $EmulatorArgs
    $emulator = Start-Process -FilePath $noGbaPath -ArgumentList $argsList `
        -WorkingDirectory $noGbaDir -PassThru

    $deadline = (Get-Date).AddSeconds([Math]::Max($DelaySeconds, 2) + 10)
    $windows = @()
    do {
        Start-Sleep -Milliseconds 250
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            $windows = @(
                [Smash64DSNoGbaWindowCapture]::GetWindowsForProcess(
                    $emulator.Id)
            )
        }
    } while ($windows.Count -eq 0 -and
             -not $emulator.HasExited -and (Get-Date) -lt $deadline)

    if ($emulator.HasExited -or $windows.Count -eq 0) {
        throw 'no$gba did not expose a capturable window.'
    }

    Start-Sleep -Seconds $DelaySeconds
    $emulator.Refresh()
    $windows = @(
        [Smash64DSNoGbaWindowCapture]::GetWindowsForProcess($emulator.Id)
    )
    if ($windows.Count -eq 0) {
        throw 'no$gba windows disappeared before capture.'
    }

    for ($i = 0; $i -lt $windows.Count; $i++) {
        Write-Output ("no`$gba window {0}: {1}x{2} title='{3}'" -f `
            $i, $windows[$i].Width, $windows[$i].Height, $windows[$i].Title)
    }

    if ($AllWindows) {
        for ($i = 0; $i -lt $windows.Count; $i++) {
            Save-WindowCapture `
                -Window $windows[$i] `
                -Index $i `
                -Path (Get-WindowOutputPath $Output $i $true)
        }
    } else {
        if (($WindowIndex -lt 0) -or ($WindowIndex -ge $windows.Count)) {
            throw "Requested no`$gba window index $WindowIndex but found $($windows.Count) window(s)."
        }
        Save-WindowCapture `
            -Window $windows[$WindowIndex] `
            -Index $WindowIndex `
            -Path (Get-WindowOutputPath $Output $WindowIndex $false)
    }
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
}
