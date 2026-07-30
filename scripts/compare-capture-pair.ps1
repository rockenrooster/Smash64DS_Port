param(
    [Parameter(Mandatory=$true)][string]$Control,
    [Parameter(Mandatory=$true)][string]$Candidate,
    # Fraction of pixels allowed to differ before the pair is called a
    # regression. 0 demands pixel-identity, which is the right bar for a change
    # that is supposed to be invisible (a memo, a skipped redundant present).
    [ValidateRange(0.0, 1.0)][double]$MaxDifferingFraction = 0.0,
    [string]$DiffImage,
    # Compare the whole window bitmap, chrome included. Off by default -- see
    # the note on window chrome below.
    [switch]$IncludeWindowChrome
)

# Compare a matched-tic capture pair and report differing pixels and the maximum
# per-channel delta.
#
# WHY THIS EXISTS. R2b, R4b and R4d each needed exactly this and each did it by
# hand. Doing it by hand is also how a pair of screenshots of the owner's BROWSER
# once got compared and read as a 67.7% visual regression (see
# capture-results-tic.ps1) -- the numbers looked like real evidence because
# nothing tied them to what was actually in the files.
#
# Reports rather than throws for a nonzero budget, so a rendering change with an
# agreed fidelity budget can record its delta; exits nonzero only when the
# measured fraction exceeds the budget.

# WINDOW CHROME IS NOT THE GUEST. The captures are of the whole melonDS window,
# and its TITLE BAR carries melonDS's own host-speed readout -- "[77/60] melonDS".
# That text changes whenever the emulator runs at a different speed, which is
# exactly what a successful optimization causes. Measured 2026-07-30 on R4d: a
# pixel-identical guest picture reported 74 differing pixels at max channel delta
# 243, entirely from the digits "77" versus "61" in the title bar. Comparing the
# full window therefore makes every genuine speedup look like a visual
# regression, and the faster the win the worse it reads.
#
# So compare the guest viewport only. The canonical profile is a 416x664 window
# whose 400x600 client area sits below a 56px title/frame band with an 8px border
# (check-melonds-policy.ps1 pins that geometry, including the exact 256x384
# dual-screen aspect).
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')

$GUEST_BORDER_X = 8
$GUEST_BAND_TOP = 56

foreach ($path in @($Control, $Candidate)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Capture is missing: $path"
    }
}

$a = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Control).Path)
$b = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Candidate).Path)
try {
    if (($a.Width -ne $b.Width) -or ($a.Height -ne $b.Height)) {
        throw ("Captures differ in size: $($a.Width)x$($a.Height) vs " +
               "$($b.Width)x$($b.Height). A size change means the two arms were " +
               'not captured through the same window profile, so no pixel ' +
               'comparison between them means anything.')
    }

    if ($IncludeWindowChrome) {
        $region = New-Object System.Drawing.Rectangle 0, 0, $a.Width, $a.Height
    } else {
        $vw = $script:MelonDSCanonicalWindowWidth - (2 * $GUEST_BORDER_X)
        $vh = $script:MelonDSCanonicalWindowHeight - $GUEST_BAND_TOP - $GUEST_BORDER_X
        if (($a.Width -ne $script:MelonDSCanonicalWindowWidth) -or
            ($a.Height -ne $script:MelonDSCanonicalWindowHeight)) {
            throw ("Captures are $($a.Width)x$($a.Height) but the canonical " +
                   "melonDS window is $($script:MelonDSCanonicalWindowWidth)x" +
                   "$($script:MelonDSCanonicalWindowHeight). The guest crop is " +
                   'derived from that profile, so cropping this pair would ' +
                   'compare the wrong rectangle. Re-capture through the ' +
                   'canonical profile, or pass -IncludeWindowChrome knowingly.')
        }
        $region = New-Object System.Drawing.Rectangle `
            $GUEST_BORDER_X, $GUEST_BAND_TOP, $vw, $vh
    }

    # LockBits, not GetPixel: a 416x664 pair is 276,224 pixels and GetPixel makes
    # that take minutes. Both are read as 32bpp ARGB regardless of source format.
    $rect = $region
    $fmt = [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
    $mode = [System.Drawing.Imaging.ImageLockMode]::ReadOnly
    $da = $a.LockBits($rect, $mode, $fmt)
    $db = $b.LockBits($rect, $mode, $fmt)
    try {
        # Copy row by row against Stride. A cropped LockBits region is a VIEW
        # into the parent bitmap, so Scan0 advances by Stride (the parent's row
        # pitch), not by region.Width*4; a flat copy of W*H*4 would walk off the
        # crop and silently compare the wrong pixels.
        $rowBytes = $region.Width * 4
        $count = $rowBytes * $region.Height
        $ba = New-Object byte[] $count
        $bb = New-Object byte[] $count
        for ($row = 0; $row -lt $region.Height; $row++) {
            [System.Runtime.InteropServices.Marshal]::Copy(
                [IntPtr]::Add($da.Scan0, $row * $da.Stride),
                $ba, $row * $rowBytes, $rowBytes)
            [System.Runtime.InteropServices.Marshal]::Copy(
                [IntPtr]::Add($db.Scan0, $row * $db.Stride),
                $bb, $row * $rowBytes, $rowBytes)
        }
    } finally {
        $a.UnlockBits($da)
        $b.UnlockBits($db)
    }

    $differing = 0
    $maxDelta = 0
    $diffMask = if ($DiffImage) {
        New-Object bool[] ($region.Width * $region.Height)
    } else { $null }
    for ($i = 0; $i -lt $count; $i += 4) {
        # Alpha is ignored: window captures carry an opaque or undefined alpha
        # channel that varies with the capture path, not with the guest picture.
        $d0 = [Math]::Abs([int]$ba[$i]     - [int]$bb[$i])
        $d1 = [Math]::Abs([int]$ba[$i + 1] - [int]$bb[$i + 1])
        $d2 = [Math]::Abs([int]$ba[$i + 2] - [int]$bb[$i + 2])
        $d = [Math]::Max($d0, [Math]::Max($d1, $d2))
        if ($d -ne 0) {
            $differing++
            if ($d -gt $maxDelta) { $maxDelta = $d }
            if ($diffMask) { $diffMask[$i / 4] = $true }
        }
    }

    $total = $region.Width * $region.Height
    $fraction = $differing / [double]$total
    Write-Host ("compared {0:N0} pixels ({1}x{2} at {3},{4}{5})" -f `
        $total, $region.Width, $region.Height, $region.X, $region.Y,
        $(if ($IncludeWindowChrome) { ' -- WINDOW CHROME INCLUDED' }
          else { ' -- guest viewport' }))
    Write-Host ("  control   {0}" -f (Split-Path -Leaf $Control))
    Write-Host ("  candidate {0}" -f (Split-Path -Leaf $Candidate))
    Write-Host ("  differing {0:N0} ({1:P4})" -f $differing, $fraction)
    Write-Host ("  max channel delta {0}" -f $maxDelta)

    if ($DiffImage -and ($differing -gt 0)) {
        $out = New-Object System.Drawing.Bitmap $region.Width, $region.Height
        for ($y = 0; $y -lt $region.Height; $y++) {
            for ($x = 0; $x -lt $region.Width; $x++) {
                $out.SetPixel($x, $y, ($(if ($diffMask[$y * $region.Width + $x]) {
                    [System.Drawing.Color]::Magenta
                } else { [System.Drawing.Color]::Black })))
            }
        }
        [void](New-Item -ItemType Directory -Force -Path (Split-Path -Parent $DiffImage))
        $out.Save($DiffImage, [System.Drawing.Imaging.ImageFormat]::Png)
        $out.Dispose()
        Write-Host "  wrote $DiffImage"
    }

    if ($fraction -gt $MaxDifferingFraction) {
        Write-Host ''
        Write-Host ("REGRESSION: {0:P4} of pixels differ, budget is {1:P4}." -f `
            $fraction, $MaxDifferingFraction)
        exit 1
    }
    Write-Host ''
    Write-Host ($(if ($differing -eq 0) { 'PIXEL-IDENTICAL.' }
                  else { 'Within the stated fidelity budget.' }))
}
finally {
    $a.Dispose()
    $b.Dispose()
}
