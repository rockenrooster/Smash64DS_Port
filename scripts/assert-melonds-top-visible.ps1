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
    [double]$MinNonWhiteNonGreenFraction = -1.0,
    [int]$RequiredRegionX = -1,
    [int]$RequiredRegionY = -1,
    [int]$RequiredRegionWidth = 0,
    [int]$RequiredRegionHeight = 0,
    [double]$MinRequiredRegionFraction = -1.0,
    [double]$MinRequiredRegionFighterFraction = -1.0,
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
function Test-DominantGreenPixel($Pixel) {
    return (($Pixel.G -ge 80) -and
        ($Pixel.G -gt ($Pixel.R + 20)) -and
        ($Pixel.G -gt ($Pixel.B + 20)))
}
function Test-NearWhitePixel($Pixel) {
    return (($Pixel.R -ge 220) -and ($Pixel.G -ge 220) -and
        ($Pixel.B -ge 220))
}
function Test-ClearPixel($Pixel) {
    return (($Pixel.R -eq $ClearR) -and ($Pixel.G -eq $ClearG) -and
        ($Pixel.B -eq $ClearB))
}
function Test-SceneDetailPixel($Pixel) {
    return ((Test-ClearPixel $Pixel) -eq $false) -and
        ((Test-DominantGreenPixel $Pixel) -eq $false) -and
        ((Test-NearWhitePixel $Pixel) -eq $false)
}
function Test-FighterDetailPixel($Pixel) {
    $range = [Math]::Max($Pixel.R, [Math]::Max($Pixel.G, $Pixel.B)) -
        [Math]::Min($Pixel.R, [Math]::Min($Pixel.G, $Pixel.B))
    $yellowStage = (($Pixel.R -gt 160) -and ($Pixel.G -gt 120) -and
        ($Pixel.B -lt 80))
    return ((Test-SceneDetailPixel $Pixel) -ne $false) -and
        ($range -gt 80) -and
        ($yellowStage -eq $false)
}
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
    $nonWhiteNonGreen = 0
    $changed = 0
    $total = $Width * $Height
    for ($y = $TopY; $y -lt ($TopY + $Height); $y++) {
        for ($x = $TopX; $x -lt ($TopX + $Width); $x++) {
            $pixel = $bitmap.GetPixel($x, $y)
            if ((Test-ClearPixel $pixel) -eq $false) {
                $different++
            }
            if (Test-DominantGreenPixel $pixel) {
                $dominantGreen++
            }
            if (Test-SceneDetailPixel $pixel) {
                $nonWhiteNonGreen++
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
    if ($MinNonWhiteNonGreenFraction -ge 0.0) {
        $detailFraction = [double]$nonWhiteNonGreen / [double]$total
        if ($detailFraction -lt $MinNonWhiteNonGreenFraction) {
            throw ("Top screen lacks non-white/non-green stage/fighter content: {0}/{1} pixels ({2:P3}) below required {3:P3}." -f
                $nonWhiteNonGreen, $total, $detailFraction,
                $MinNonWhiteNonGreenFraction)
        }
        Write-Output ("Top screen detail content: {0}/{1} non-white/non-green pixels ({2:P3})." -f
            $nonWhiteNonGreen, $total, $detailFraction)
    }
    if (($MinRequiredRegionFraction -ge 0.0) -or
        ($MinRequiredRegionFighterFraction -ge 0.0)) {
        if (($RequiredRegionX -lt 0) -or ($RequiredRegionY -lt 0) -or
            ($RequiredRegionWidth -le 0) -or ($RequiredRegionHeight -le 0)) {
            throw 'Required region parameters are incomplete.'
        }
        if (($RequiredRegionX + $RequiredRegionWidth) -gt $Width -or
            ($RequiredRegionY + $RequiredRegionHeight) -gt $Height) {
            throw "Required top-screen region ${RequiredRegionX},${RequiredRegionY} ${RequiredRegionWidth}x${RequiredRegionHeight} exceeds crop ${Width}x${Height}."
        }
        $regionDetail = 0
        $regionFighterDetail = 0
        $regionTotal = $RequiredRegionWidth * $RequiredRegionHeight
        for ($ry = ($TopY + $RequiredRegionY); $ry -lt ($TopY + $RequiredRegionY + $RequiredRegionHeight); $ry++) {
            for ($rx = ($TopX + $RequiredRegionX); $rx -lt ($TopX + $RequiredRegionX + $RequiredRegionWidth); $rx++) {
                $regionPixel = $bitmap.GetPixel($rx, $ry)
                if (Test-SceneDetailPixel $regionPixel) {
                    $regionDetail++
                }
                if (Test-FighterDetailPixel $regionPixel) {
                    $regionFighterDetail++
                }
            }
        }
        $regionFraction = [double]$regionDetail / [double]$regionTotal
        if (($MinRequiredRegionFraction -ge 0.0) -and
            ($regionFraction -lt $MinRequiredRegionFraction)) {
            throw ("Top-screen required region lacks fighter/stage detail: {0}/{1} pixels ({2:P3}) below required {3:P3}." -f
                $regionDetail, $regionTotal, $regionFraction,
                $MinRequiredRegionFraction)
        }
        if ($MinRequiredRegionFraction -ge 0.0) {
            Write-Output ("Top-screen required region detail: {0}/{1} pixels ({2:P3})." -f
                $regionDetail, $regionTotal, $regionFraction)
        }
        $regionFighterFraction = [double]$regionFighterDetail /
            [double]$regionTotal
        if (($MinRequiredRegionFighterFraction -ge 0.0) -and
            ($regionFighterFraction -lt $MinRequiredRegionFighterFraction)) {
            throw ("Top-screen required region lacks fighter-colored pixels: {0}/{1} pixels ({2:P3}) below required {3:P3}." -f
                $regionFighterDetail, $regionTotal, $regionFighterFraction,
                $MinRequiredRegionFighterFraction)
        }
        if ($MinRequiredRegionFighterFraction -ge 0.0) {
            Write-Output ("Top-screen fighter-region color: {0}/{1} pixels ({2:P3})." -f
                $regionFighterDetail, $regionTotal, $regionFighterFraction)
        }
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
