Add-Type -AssemblyName System.Drawing
if (-not ('Smash64DSWindowCapture' -as [type])) {
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
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool PrintWindow(
        IntPtr window, IntPtr destination, uint flags);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(
        IntPtr window, IntPtr insertAfter, int x, int y, int width, int height,
        uint flags);
}
'@
}

# Grab the melonDS window into a Bitmap. CopyFromScreen is preferred because it
# reproduces what the owner sees; PrintWindow is the fallback for a session with
# no desktop surface. Callers own the returned bitmap.
function Get-MelonDSWindowBitmap {
    param([Parameter(Mandatory=$true)][System.IntPtr]$WindowHandle)

    $rect = New-Object Smash64DSWindowCapture+Rect
    if (-not [Smash64DSWindowCapture]::GetWindowRect($WindowHandle, [ref]$rect)) {
        throw 'Could not read the melonDS window bounds.'
    }
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if (($width -le 0) -or ($height -le 0)) {
        throw "Invalid melonDS window bounds: ${width}x${height}."
    }
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        try {
            $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        } catch {
            $screenError = $_.Exception.Message
            $destination = $graphics.GetHdc()
            try {
                if (-not [Smash64DSWindowCapture]::PrintWindow(
                        $WindowHandle, $destination, 2)) {
                    throw "CopyFromScreen failed ($screenError) and PrintWindow failed."
                }
            } finally {
                $graphics.ReleaseHdc($destination)
            }
        }
    } catch {
        $bitmap.Dispose()
        throw
    } finally {
        $graphics.Dispose()
    }
    return $bitmap
}

# SHA-256 over the raw window pixels. Two identical hashes seconds apart mean
# nothing on either DS screen changed, which is how a human sees a freeze -- and
# it needs no GDB, which matters because melonDS's stub serves exactly ONE
# session per emulation run (see KNOWN_ISSUES): a polled-GDB watchdog is not
# buildable, and a breakpoint-driven one runs the ROM about twelve times slower
# than real time and so changes the timing of anything race-shaped.
function Get-MelonDSWindowFrameHash {
    param([Parameter(Mandatory=$true)][System.IntPtr]$WindowHandle)

    $bitmap = Get-MelonDSWindowBitmap -WindowHandle $WindowHandle
    try {
        $data = $bitmap.LockBits(
            (New-Object System.Drawing.Rectangle 0, 0, $bitmap.Width, $bitmap.Height),
            [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
            [System.Drawing.Imaging.PixelFormat]::Format32bppRgb)
        try {
            $bytes = New-Object byte[] ([Math]::Abs($data.Stride) * $data.Height)
            [System.Runtime.InteropServices.Marshal]::Copy(
                $data.Scan0, $bytes, 0, $bytes.Length)
        } finally {
            $bitmap.UnlockBits($data)
        }
        $sha = [System.Security.Cryptography.SHA256]::Create()
        try {
            return [System.BitConverter]::ToString($sha.ComputeHash($bytes)).Replace('-', '')
        } finally {
            $sha.Dispose()
        }
    } finally {
        $bitmap.Dispose()
    }
}

function Save-MelonDSWindowCapture {
    param(
        [Parameter(Mandatory=$true)][System.IntPtr]$WindowHandle,
        [Parameter(Mandatory=$true)][string]$Path
    )

    $bitmap = Get-MelonDSWindowBitmap -WindowHandle $WindowHandle
    try {
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
        return "$($bitmap.Width)x$($bitmap.Height)"
    } finally {
        $bitmap.Dispose()
    }
}

function Convert-MelonDSWindowTopToNativeBitmap {
    param(
        [Parameter(Mandatory=$true)]
        [System.Drawing.Bitmap]$Bitmap,
        [int]$Width = 256,
        [int]$Height = 192,
        [int]$TopX = 8,
        [int]$TopY = 56,
        [switch]$WindowScaledCapture
    )

    $left = [double]$TopX
    $top = [double]$TopY
    $scale = 1.0
    if ($WindowScaledCapture) {
        # Set-MelonDSCaptureWindow preserves a stacked 256x192 + 256x192
        # layout inside the Windows client area. Account for the 8px frame and
        # fixed title/menu height, then sample each native pixel at cell center.
        $availableWidth = $Bitmap.Width - 16
        $availableHeight = $Bitmap.Height - $TopY - 8
        $scale = [Math]::Min(
            [double]$availableWidth / [double]$Width,
            [double]$availableHeight / [double]($Height * 2))
        if ($scale -le 0.0) {
            throw "Invalid scaled melonDS window geometry $($Bitmap.Width)x$($Bitmap.Height)."
        }
        $left = ([double]$Bitmap.Width - ([double]$Width * $scale)) / 2.0
    } elseif (($TopX + $Width) -gt $Bitmap.Width -or
              ($TopY + $Height) -gt $Bitmap.Height) {
        throw "Top-screen crop ${TopX},${TopY} ${Width}x${Height} exceeds image $($Bitmap.Width)x$($Bitmap.Height)."
    }

    $native = New-Object System.Drawing.Bitmap $Width, $Height
    try {
        for ($y = 0; $y -lt $Height; $y++) {
            $sourceY = [Math]::Floor($top + (($y + 0.5) * $scale))
            for ($x = 0; $x -lt $Width; $x++) {
                $sourceX = [Math]::Floor($left + (($x + 0.5) * $scale))
                if (($sourceX -lt 0) -or ($sourceY -lt 0) -or
                    ($sourceX -ge $Bitmap.Width) -or
                    ($sourceY -ge $Bitmap.Height)) {
                    throw "Scaled top-screen sample ${sourceX},${sourceY} exceeds image $($Bitmap.Width)x$($Bitmap.Height)."
                }
                $native.SetPixel($x, $y, $Bitmap.GetPixel($sourceX, $sourceY))
            }
        }
        return $native
    } catch {
        $native.Dispose()
        throw
    }
}
