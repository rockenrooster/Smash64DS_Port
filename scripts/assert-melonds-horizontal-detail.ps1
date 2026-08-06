param(
    [Parameter(Mandatory=$true)]
    [string]$Image,
    [int]$TopX = 8,
    [int]$TopY = 56,
    [int]$Width = 256,
    [int]$Height = 192,
    [int]$ChannelThreshold = 20,
    [switch]$WindowScaledCapture,
    [Parameter(Mandatory=$true)]
    [string[]]$Region
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path $Image)) {
    throw "Screenshot not found: $Image"
}
if ($ChannelThreshold -lt 1) {
    throw 'ChannelThreshold must be positive.'
}
Add-Type -AssemblyName System.Drawing
. (Join-Path $PSScriptRoot 'lib\melonds-screenshot.ps1')
function Convert-RegionSpec($Spec) {
    if ($Spec -notmatch '^([^:]+):([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]*\.?[0-9]+),([0-9]+)$') {
        throw "Invalid detail region '$Spec'. Expected name:x,y,width,height,minVariation,maxFlatRun."
    }
    return [pscustomobject]@{
        Name = $matches[1]
        X = [int]$matches[2]
        Y = [int]$matches[3]
        Width = [int]$matches[4]
        Height = [int]$matches[5]
        MinVariation = [double]$matches[6]
        MaxFlatRun = [int]$matches[7]
    }
}
function Measure-HorizontalDetail {
    param(
        [Parameter(Mandatory=$true)]$Bitmap,
        [Parameter(Mandatory=$true)]$Spec
    )
    if (($Spec.X -lt 0) -or ($Spec.Y -lt 0) -or
        ($Spec.Width -lt 2) -or ($Spec.Height -le 0) -or
        (($Spec.X + $Spec.Width) -gt $Width) -or
        (($Spec.Y + $Spec.Height) -gt $Height)) {
        throw "Top-screen detail region $($Spec.Name) $($Spec.X),$($Spec.Y) $($Spec.Width)x$($Spec.Height) exceeds crop ${Width}x${Height}."
    }
    $varied = 0
    $pairs = ($Spec.Width - 1) * $Spec.Height
    $maxFlatRun = 1
    for ($y = 0; $y -lt $Spec.Height; $y++) {
        $flatRun = 1
        $previous = $Bitmap.GetPixel(
            $Spec.X,
            $Spec.Y + $y)
        for ($x = 1; $x -lt $Spec.Width; $x++) {
            $current = $Bitmap.GetPixel(
                $Spec.X + $x,
                $Spec.Y + $y)
            $delta = [Math]::Max(
                [Math]::Abs([int]$current.R - [int]$previous.R),
                [Math]::Max(
                    [Math]::Abs([int]$current.G - [int]$previous.G),
                    [Math]::Abs([int]$current.B - [int]$previous.B)))
            if ($delta -ge $ChannelThreshold) {
                $varied++
                $flatRun = 1
            } else {
                $flatRun++
                if ($flatRun -gt $maxFlatRun) {
                    $maxFlatRun = $flatRun
                }
            }
            $previous = $current
        }
    }
    $variation = [double]$varied / [double]$pairs
    $flatRunGate = if ($Spec.MaxFlatRun -gt 0) {
        "limit=$($Spec.MaxFlatRun)px"
    } else {
        'observed-only'
    }
    Write-Output ("Horizontal detail {0}: varied={1}/{2} ({3:P3}) max-flat-run={4}px ({5})." -f
        $Spec.Name, $varied, $pairs, $variation, $maxFlatRun, $flatRunGate)
    if ($variation -lt $Spec.MinVariation) {
        # ALWAYS name the source capture's geometry in the failure. This gate
        # measures adjacent-pixel deltas, so it is a function of capture
        # RESOLUTION as much as of what the ROM drew: a window captured at a
        # lower effective scale averages neighbouring texels and drops the
        # variation without a single rendered pixel changing. On 2026-08-05 six
        # captures at 877x1400 passed this exact region and two at 620x1212
        # failed it (29.6%/31.3%) with a visually identical, correctly rendered
        # scene -- and the bare percentage read as a rendering regression for
        # four verifier runs. Print the geometry so the next reader can compare
        # it against a passing capture before suspecting the ROM.
        # NOTE the parentheses around the concatenation: -f binds TIGHTER than
        # +, so `"a" + "b" -f $x` formats only "b" and emits the rest with its
        # placeholders raw -- which is exactly what this message did on its
        # first draft.
        throw (("Horizontal detail region {0} variation {1:P3} is below " +
            "required {2:P3}. Source capture {3} is {4}x{5}; this gate scales " +
            "with capture resolution, so compare that against a passing " +
            "capture before attributing it to the ROM.") -f
            $Spec.Name, $variation, $Spec.MinVariation,
            $script:DetailSourceImageName, $script:DetailSourceImageWidth,
            $script:DetailSourceImageHeight)
    }
    if (($Spec.MaxFlatRun -gt 0) -and
        ($maxFlatRun -gt $Spec.MaxFlatRun)) {
        throw ("Horizontal detail region {0} has a {1}px flat run above allowed {2}px." -f
            $Spec.Name, $maxFlatRun, $Spec.MaxFlatRun)
    }
}
$windowBitmap = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Image).Path)
# Captured for the failure message above; see the comment there for why.
$script:DetailSourceImageName = Split-Path -Leaf $Image
$script:DetailSourceImageWidth = $windowBitmap.Width
$script:DetailSourceImageHeight = $windowBitmap.Height
$bitmap = $null
try {
    $bitmap = Convert-MelonDSWindowTopToNativeBitmap `
        -Bitmap $windowBitmap -Width $Width -Height $Height `
        -TopX $TopX -TopY $TopY `
        -WindowScaledCapture:$WindowScaledCapture
    foreach ($specText in $Region) {
        Measure-HorizontalDetail -Bitmap $bitmap -Spec (Convert-RegionSpec $specText)
    }
} finally {
    if ($null -ne $bitmap) {
        $bitmap.Dispose()
    }
    $windowBitmap.Dispose()
}
