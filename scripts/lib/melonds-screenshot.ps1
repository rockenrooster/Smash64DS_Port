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
    [StructLayout(LayoutKind.Sequential)]
    public struct Point
    {
        public int X;
        public int Y;
    }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(IntPtr window, ref Point point);
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
    // Windows grants SetForegroundWindow only to the process that already owns
    // the foreground, so on a machine somebody is USING it silently does
    // nothing. Callers that capture pixels as EVIDENCE must read this back and
    // refuse to write a file when the raise was denied -- see the
    // CopyFromScreen note in Get-MelonDSWindowBitmap for what happens if they
    // do not.
    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();
    // Synthesises a HELD key, which SendKeys cannot do. A guest sampling input
    // once per rendered frame needs the key down across at least one sample, and
    // the Results screen renders at roughly 6 FPS -- so a SendKeys tap is very
    // likely to fall entirely between two samples and never register as a
    // button_tap edge. Measured exactly that on 2026-07-30.
    [DllImport("user32.dll")]
    public static extern void keybd_event(
        byte key, byte scan, uint flags, UIntPtr extra);
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
#
# -ClientOnly excludes the window chrome. Use it for ANY measurement that asks
# whether the GUEST changed, never the default full-window grab: see the trap
# recorded above Get-MelonDSWindowFrameHash.
function Get-MelonDSWindowBitmap {
    param(
        [Parameter(Mandatory=$true)][System.IntPtr]$WindowHandle,
        [switch]$ClientOnly,
        # Ask the window to render ITSELF instead of copying the desktop region
        # it occupies. Immune to occlusion and to whatever else the operator has
        # on screen, so it needs no foreground raise -- which is the only way to
        # capture evidence reliably on a machine somebody is using, and the only
        # way that cannot accidentally photograph their desktop. Not the default
        # because a GPU-composited window can answer PrintWindow with a blank
        # surface; callers using this must check the result is not uniform.
        [switch]$PreferPrintWindow
    )

    $rect = New-Object Smash64DSWindowCapture+Rect
    $origin = New-Object Smash64DSWindowCapture+Point
    if ($ClientOnly) {
        if (-not [Smash64DSWindowCapture]::GetClientRect($WindowHandle, [ref]$rect)) {
            throw 'Could not read the melonDS client bounds.'
        }
        # GetClientRect is client-relative, so (0,0) needs mapping to the screen
        # before CopyFromScreen can find it.
        if (-not [Smash64DSWindowCapture]::ClientToScreen($WindowHandle, [ref]$origin)) {
            throw 'Could not map the melonDS client origin to the screen.'
        }
    } else {
        if (-not [Smash64DSWindowCapture]::GetWindowRect($WindowHandle, [ref]$rect)) {
            throw 'Could not read the melonDS window bounds.'
        }
        $origin.X = $rect.Left
        $origin.Y = $rect.Top
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
            # CAUTION -- THIS READS THE DESKTOP, NOT THE WINDOW. CopyFromScreen
            # copies whatever pixels currently occupy that screen rectangle, so
            # anything stacked over melonDS is what lands in the bitmap. It does
            # NOT throw when the window is occluded, which is why the PrintWindow
            # fallback below never rescues that case -- the fallback only covers
            # CopyFromScreen erroring outright.
            #
            # Measured 2026-07-30: a new capture harness took two "matched-tic"
            # screenshots of the owner's BROWSER while melonDS sat behind it. The
            # pair diffed at 67.7% of pixels with a max channel delta of 255 and
            # read exactly like a catastrophic visual regression in the change
            # under test. Nothing in the pipeline flagged it; only looking at the
            # image did.
            #
            # This was once qualified with "liveness hashing tolerates it, since
            # a stable occluder just reads as a frozen picture, which is already
            # the failure verdict". WRONG, and withdrawn 2026-08-01: a stable
            # occluder does not degrade a liveness verdict, it manufactures a
            # freeze that never happened. See Get-MelonDSWindowFrameHash, which
            # now passes -PreferPrintWindow for exactly that reason. Treat every
            # CopyFromScreen caller as suspect, not just the ones writing files.
            #
            # EVIDENCE capture is the stricter case still. Any caller writing a
            # file for the owner to judge
            # must first raise the window AND verify with GetForegroundWindow
            # that the raise was granted, then refuse to write if it was not.
            # `capture-results-tic.ps1` does exactly that; copy it, do not
            # reinvent it.
            if ($PreferPrintWindow) {
                $destination = $graphics.GetHdc()
                try {
                    $flags = if ($ClientOnly) { 3 } else { 2 }
                    if (-not [Smash64DSWindowCapture]::PrintWindow(
                            $WindowHandle, $destination, $flags)) {
                        throw 'PrintWindow refused the window.'
                    }
                } finally {
                    $graphics.ReleaseHdc($destination)
                }
                return $bitmap
            }
            $graphics.CopyFromScreen($origin.X, $origin.Y, 0, 0, $bitmap.Size)
        } catch {
            $screenError = $_.Exception.Message
            $destination = $graphics.GetHdc()
            try {
                # PW_RENDERFULLCONTENT, plus PW_CLIENTONLY when the caller asked
                # for the client area, so the fallback crops the same region the
                # primary path does.
                $flags = if ($ClientOnly) { 3 } else { 2 }
                if (-not [Smash64DSWindowCapture]::PrintWindow(
                        $WindowHandle, $destination, $flags)) {
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

# SHA-256 over the GUEST pixels only. Two identical hashes seconds apart mean
# nothing on either DS screen changed, which is how a human sees a freeze -- and
# it needs no GDB, which matters because melonDS's stub serves exactly ONE
# session per emulation run (see KNOWN_ISSUES): a polled-GDB watchdog is not
# buildable, and a breakpoint-driven one runs the ROM about twelve times slower
# than real time and so changes the timing of anything race-shaped.
#
# -ClientOnly IS THE WHOLE POINT AND IS NOT OPTIONAL. This hashed GetWindowRect
# until 2026-07-29, which includes the title bar, and melonDS renders its FPS
# counter into the title ("[83/60] melonDS ..."). A completely hung ARM9 therefore
# produced a different hash on every single poll, because the HOST kept counting
# frames it was presenting from a dead guest. The detector reported "alive, 54
# distinct frames" across ten minutes of a ROM frozen on its boot screen, and two
# soak verdicts had to be withdrawn -- one of them a 150/150-distinct "clean"
# result whose perfection was the tell, since a live game repeats a frame now and
# then and a ticking counter never does. The window frame also let the desktop
# bleed in at the edges. Never widen this back to the window rect: a liveness
# metric must not be able to see anything the guest does not draw.
#
# -PreferPrintWindow IS ALSO NOT OPTIONAL, for the same class of reason. The
# comment in Get-MelonDSWindowBitmap used to argue that liveness hashing could
# safely keep CopyFromScreen because "a stable occluder just reads as a frozen
# picture, which is already the failure verdict". That is wrong, and 2026-08-01
# is the proof: the owner had a browser over melonDS, all eight polls hashed the
# BROWSER, and the run reported FROZEN-FROM-START against a ROM that was running
# perfectly -- 1,721 presented frames and a healthy heap in the same capture.
# An occluder does not degrade the verdict conservatively, it FABRICATES one,
# and the attached GDB stack then invites a hunt for a hang at whatever PC a
# running ROM happened to be interrupted at. It also wrote a photograph of the
# owner's desktop into artifacts/. Read the window, never the screen.
function Get-MelonDSWindowFrameHash {
    param([Parameter(Mandatory=$true)][System.IntPtr]$WindowHandle)

    $bitmap = Get-MelonDSWindowBitmap -WindowHandle $WindowHandle -ClientOnly `
        -PreferPrintWindow
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

# How many distinct colours a sparse grid over the guest screens sees. This is
# the discriminator between "the capture is broken" and "the guest died before it
# drew anything": a failed grab is uniform (all black, or one desktop colour),
# while a ROM hung during boot still shows real boot pixels. Needed because the
# chrome-free hash removed the spurious motion that used to make a dead-from-boot
# ROM look alive, so "never moved" became a common and genuine verdict.
function Measure-MelonDSWindowDistinctColors {
    param(
        [Parameter(Mandatory=$true)][System.IntPtr]$WindowHandle,
        [int]$Step = 16
    )

    # Same window-not-screen rule as the hash it discriminates for: if this read
    # the desktop it would count the OCCLUDER's colours and cheerfully confirm
    # "20 distinct colours, so the ROM drew a frame and then stopped".
    $bitmap = Get-MelonDSWindowBitmap -WindowHandle $WindowHandle -ClientOnly `
        -PreferPrintWindow
    try {
        $seen = New-Object 'System.Collections.Generic.HashSet[int]'
        for ($y = 0; $y -lt $bitmap.Height; $y += $Step) {
            for ($x = 0; $x -lt $bitmap.Width; $x += $Step) {
                [void]$seen.Add($bitmap.GetPixel($x, $y).ToArgb())
            }
        }
        return $seen.Count
    } finally {
        $bitmap.Dispose()
    }
}

function Save-MelonDSWindowCapture {
    param(
        [Parameter(Mandatory=$true)][System.IntPtr]$WindowHandle,
        [Parameter(Mandatory=$true)][string]$Path,
        # See Get-MelonDSWindowBitmap. Evidence captures should pass this: it
        # reads the window rather than the desktop, so no foreground raise is
        # needed and an occluding window cannot be photographed by mistake.
        [switch]$PreferPrintWindow
    )

    $bitmap = Get-MelonDSWindowBitmap -WindowHandle $WindowHandle `
        -PreferPrintWindow:$PreferPrintWindow
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
