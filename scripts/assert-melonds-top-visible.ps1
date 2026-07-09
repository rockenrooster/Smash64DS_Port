param(
    [Parameter(Mandatory=$true)]
    [string]$Image,
    [string]$CompareImage,
    [int]$TopX = 8,
    [int]$TopY = 56,
    [int]$Width = 256,
    [int]$Height = 192,
    [int]$ClearR = 20,
    [int]$ClearG = 28,
    [int]$ClearB = 52,
    [double]$MinDifferentFraction = 0.01,
    [double]$MinDominantGreenFraction = -1.0,
    [double]$MinCompareChangedFraction = -1.0,
    [double]$MaxCompareChangedFraction = -1.0
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path $Image)) {
    throw "Screenshot not found: $Image"
}
if (-not [string]::IsNullOrWhiteSpace($CompareImage) -and
    -not (Test-Path $CompareImage)) {
    throw "Comparison screenshot not found: $CompareImage"
}
Add-Type -AssemblyName System.Drawing
$bitmap = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Image).Path)
$compareBitmap = $null
try {
    if (($TopX + $Width) -gt $bitmap.Width -or
        ($TopY + $Height) -gt $bitmap.Height) {
        throw "Top-screen crop ${TopX},${TopY} ${Width}x${Height} exceeds image $($bitmap.Width)x$($bitmap.Height)."
    }
    if (-not [string]::IsNullOrWhiteSpace($CompareImage)) {
        $compareBitmap = [System.Drawing.Bitmap]::FromFile(
            (Resolve-Path $CompareImage).Path)
        if (($TopX + $Width) -gt $compareBitmap.Width -or
            ($TopY + $Height) -gt $compareBitmap.Height) {
            throw "Top-screen crop ${TopX},${TopY} ${Width}x${Height} exceeds comparison image $($compareBitmap.Width)x$($compareBitmap.Height)."
        }
    }
    $different = 0
    $dominantGreen = 0
    $changed = 0
    $total = $Width * $Height
    for ($y = $TopY; $y -lt ($TopY + $Height); $y++) {
        for ($x = $TopX; $x -lt ($TopX + $Width); $x++) {
            $pixel = $bitmap.GetPixel($x, $y)
            if (($pixel.R -ne $ClearR) -or
                ($pixel.G -ne $ClearG) -or
                ($pixel.B -ne $ClearB)) {
                $different++
            }
            if (($pixel.G -ge 80) -and
                ($pixel.G -gt ($pixel.R + 20)) -and
                ($pixel.G -gt ($pixel.B + 20))) {
                $dominantGreen++
            }
            if ($null -ne $compareBitmap) {
                $other = $compareBitmap.GetPixel($x, $y)
                if (($pixel.R -ne $other.R) -or
                    ($pixel.G -ne $other.G) -or
                    ($pixel.B -ne $other.B)) {
                    $changed++
                }
            }
        }
    }
    $fraction = [double]$different / [double]$total
    if ($fraction -lt $MinDifferentFraction) {
        throw ("Top screen remained at clear color: {0}/{1} different pixels ({2:P3}) below required {3:P3}." -f
            $different, $total, $fraction, $MinDifferentFraction)
    }
    Write-Output ("Top screen visible: {0}/{1} different pixels ({2:P3})." -f
        $different, $total, $fraction)
    if ($MinDominantGreenFraction -ge 0.0) {
        $greenFraction = [double]$dominantGreen / [double]$total
        if ($greenFraction -lt $MinDominantGreenFraction) {
            throw ("Top screen lacks Dream Land green content: {0}/{1} dominant-green pixels ({2:P3}) below required {3:P3}." -f
                $dominantGreen, $total, $greenFraction, $MinDominantGreenFraction)
        }
        Write-Output ("Top screen green content: {0}/{1} dominant-green pixels ({2:P3})." -f
            $dominantGreen, $total, $greenFraction)
    }
    if ($null -ne $compareBitmap) {
        $changedFraction = [double]$changed / [double]$total
        if (($MinCompareChangedFraction -ge 0.0) -and
            ($changedFraction -lt $MinCompareChangedFraction)) {
            throw ("Top screen delta too small: {0}/{1} changed pixels ({2:P3}) below required {3:P3}." -f
                $changed, $total, $changedFraction, $MinCompareChangedFraction)
        }
        if (($MaxCompareChangedFraction -ge 0.0) -and
            ($changedFraction -gt $MaxCompareChangedFraction)) {
            throw ("Top screen delta too large: {0}/{1} changed pixels ({2:P3}) above allowed {3:P3}." -f
                $changed, $total, $changedFraction, $MaxCompareChangedFraction)
        }
        Write-Output ("Top screen delta: {0}/{1} changed pixels ({2:P3})." -f
            $changed, $total, $changedFraction)
    }
} finally {
    if ($null -ne $compareBitmap) {
        $compareBitmap.Dispose()
    }
    $bitmap.Dispose()
}
