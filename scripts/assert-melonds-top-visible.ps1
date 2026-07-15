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
    [double]$MaxSingleColorFraction = -1.0,
    [int]$RequiredRegionX = -1,
    [int]$RequiredRegionY = -1,
    [int]$RequiredRegionWidth = 0,
    [int]$RequiredRegionHeight = 0,
    [double]$MinRequiredRegionFraction = -1.0,
    [double]$MinRequiredRegionFighterFraction = -1.0,
    [double]$MinCompareChangedFraction = -1.0,
    [double]$MaxCompareChangedFraction = -1.0,
    [int]$CompareChannelThreshold = 1,
    [double]$MaxCompareMeanChannelDelta = -1.0,
    [switch]$RegisterCompareCamera,
    [double]$MinCompareOverlapFraction = 0.95,
    [switch]$WindowScaledCapture,
    [string[]]$NamedRegion = @()
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
. (Join-Path $PSScriptRoot 'lib\melonds-screenshot.ps1')
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
function Find-TopCompareRegistration {
    param(
        [Parameter(Mandatory=$true)]$Bitmap,
        [Parameter(Mandatory=$true)]$CompareBitmap,
        [int]$ChannelThreshold
    )
    $centerX = ([double]$Width - 1.0) / 2.0
    $centerY = ([double]$Height - 1.0) / 2.0
    $best = $null
    for ($scaleStep = -2; $scaleStep -le 2; $scaleStep++) {
        $scale = 1.0 + ([double]$scaleStep * 0.01)
        for ($offsetY = -2; $offsetY -le 2; $offsetY++) {
            for ($offsetX = -2; $offsetX -le 2; $offsetX++) {
                $changed = 0
                $compared = 0
                [long]$deltaSum = 0
                for ($y = 2; $y -lt ($Height - 2); $y += 4) {
                    $compareY = [Math]::Round(
                        $centerY + (($y - $centerY) * $scale) + $offsetY)
                    if (($compareY -lt 0) -or ($compareY -ge $Height)) {
                        continue
                    }
                    for ($x = 2; $x -lt ($Width - 2); $x += 4) {
                        $compareX = [Math]::Round(
                            $centerX + (($x - $centerX) * $scale) +
                            $offsetX)
                        if (($compareX -lt 0) -or ($compareX -ge $Width)) {
                            continue
                        }
                        $pixel = $Bitmap.GetPixel($x, $y)
                        $other = $CompareBitmap.GetPixel($compareX, $compareY)
                        $delta = [Math]::Max(
                            [Math]::Abs([int]$pixel.R - [int]$other.R),
                            [Math]::Max(
                                [Math]::Abs([int]$pixel.G - [int]$other.G),
                                [Math]::Abs([int]$pixel.B - [int]$other.B)))
                        if ($delta -ge $ChannelThreshold) { $changed++ }
                        $deltaSum += $delta
                        $compared++
                    }
                }
                if ($compared -eq 0) { continue }
                $score = [double]$changed / [double]$compared
                $mean = [double]$deltaSum / [double]$compared
                if (($null -eq $best) -or ($score -lt $best.Score) -or
                    (($score -eq $best.Score) -and ($mean -lt $best.Mean))) {
                    $best = [pscustomobject]@{
                        Scale = $scale
                        OffsetX = $offsetX
                        OffsetY = $offsetY
                        Score = $score
                        Mean = $mean
                    }
                }
            }
        }
    }
    if ($null -eq $best) {
        throw 'Could not register comparison screenshot camera motion.'
    }
    return $best
}
function Convert-NamedRegionSpec($Spec) {
    if ($Spec -notmatch '^([^:]+):([0-9]+),([0-9]+),([0-9]+),([0-9]+)$') {
        throw "Invalid named region '$Spec'. Expected name:x,y,width,height."
    }
    return [pscustomobject]@{
        Name = $matches[1]
        X = [int]$matches[2]
        Y = [int]$matches[3]
        Width = [int]$matches[4]
        Height = [int]$matches[5]
    }
}
function Measure-TopRegion {
    param(
        [Parameter(Mandatory=$true)]$Bitmap,
        $CompareBitmap,
        [string]$Name,
        [int]$RegionX,
        [int]$RegionY,
        [int]$RegionWidth,
        [int]$RegionHeight
    )
    if (($RegionX -lt 0) -or ($RegionY -lt 0) -or
        ($RegionWidth -le 0) -or ($RegionHeight -le 0) -or
        (($RegionX + $RegionWidth) -gt $Width) -or
        (($RegionY + $RegionHeight) -gt $Height)) {
        throw "Named top-screen region ${Name} ${RegionX},${RegionY} ${RegionWidth}x${RegionHeight} exceeds crop ${Width}x${Height}."
    }
    $different = 0
    $dominantGreen = 0
    $sceneDetail = 0
    $fighterDetail = 0
    $changed = 0
    $total = $RegionWidth * $RegionHeight
    for ($ry = $RegionY; $ry -lt ($RegionY + $RegionHeight); $ry++) {
        for ($rx = $RegionX; $rx -lt ($RegionX + $RegionWidth); $rx++) {
            $pixel = $Bitmap.GetPixel($rx, $ry)
            if ((Test-ClearPixel $pixel) -eq $false) { $different++ }
            if (Test-DominantGreenPixel $pixel) { $dominantGreen++ }
            if (Test-SceneDetailPixel $pixel) { $sceneDetail++ }
            if (Test-FighterDetailPixel $pixel) { $fighterDetail++ }
            if ($null -ne $CompareBitmap) {
                $other = $CompareBitmap.GetPixel($rx, $ry)
                $delta = [Math]::Max(
                    [Math]::Abs([int]$pixel.R - [int]$other.R),
                    [Math]::Max(
                        [Math]::Abs([int]$pixel.G - [int]$other.G),
                        [Math]::Abs([int]$pixel.B - [int]$other.B)))
                if ($delta -ge $CompareChannelThreshold) {
                    $changed++
                }
            }
        }
    }
    $message = ("Region {0}: non-clear={1}/{2} ({3:P3}) green={4}/{2} ({5:P3}) detail={6}/{2} ({7:P3}) fighter={8}/{2} ({9:P3})" -f
        $Name,
        $different, $total, ([double]$different / [double]$total),
        $dominantGreen, ([double]$dominantGreen / [double]$total),
        $sceneDetail, ([double]$sceneDetail / [double]$total),
        $fighterDetail, ([double]$fighterDetail / [double]$total))
    if ($null -ne $CompareBitmap) {
        $message += (" changed={0}/{1} ({2:P3})" -f
            $changed, $total, ([double]$changed / [double]$total))
    }
    Write-Output $message
}
$windowBitmap = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Image).Path)
$bitmap = $null
$compareWindowBitmap = $null
$compareBitmap = $null
try {
    $bitmap = Convert-MelonDSWindowTopToNativeBitmap `
        -Bitmap $windowBitmap -Width $Width -Height $Height `
        -TopX $TopX -TopY $TopY `
        -WindowScaledCapture:$WindowScaledCapture
    if (-not [string]::IsNullOrWhiteSpace($CompareImage)) {
        $compareWindowBitmap = [System.Drawing.Bitmap]::FromFile(
            (Resolve-Path $CompareImage).Path)
        $compareBitmap = Convert-MelonDSWindowTopToNativeBitmap `
            -Bitmap $compareWindowBitmap -Width $Width -Height $Height `
            -TopX $TopX -TopY $TopY `
            -WindowScaledCapture:$WindowScaledCapture
    }
    $compareRegistration = $null
    if (($null -ne $compareBitmap) -and $RegisterCompareCamera) {
        $compareRegistration = Find-TopCompareRegistration `
            -Bitmap $bitmap -CompareBitmap $compareBitmap `
            -ChannelThreshold $CompareChannelThreshold
        Write-Output ("Top screen camera registration: scale={0:F2} dx={1} dy={2} sample={3:P3}." -f
            $compareRegistration.Scale,
            $compareRegistration.OffsetX,
            $compareRegistration.OffsetY,
            $compareRegistration.Score)
    }
    $different = 0
    $dominantGreen = 0
    $nonWhiteNonGreen = 0
    $changed = 0
    $rawChanged = 0
    $compareTotal = 0
    [long]$compareMaxDeltaSum = 0
    $total = $Width * $Height
    $colorHistogram = if ($MaxSingleColorFraction -ge 0.0) {
        @{}
    } else {
        $null
    }
    for ($y = 0; $y -lt $Height; $y++) {
        for ($x = 0; $x -lt $Width; $x++) {
            $pixel = $bitmap.GetPixel($x, $y)
            if ($null -ne $colorHistogram) {
                $colorKey = $pixel.ToArgb()
                if ($colorHistogram.ContainsKey($colorKey)) {
                    $colorHistogram[$colorKey]++
                } else {
                    $colorHistogram[$colorKey] = 1
                }
            }
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
                $compareX = $x
                $compareY = $y
                if ($null -ne $compareRegistration) {
                    $centerX = ([double]$Width - 1.0) / 2.0
                    $centerY = ([double]$Height - 1.0) / 2.0
                    $compareX = [Math]::Round(
                        $centerX + (($x - $centerX) *
                        $compareRegistration.Scale) +
                        $compareRegistration.OffsetX)
                    $compareY = [Math]::Round(
                        $centerY + (($y - $centerY) *
                        $compareRegistration.Scale) +
                        $compareRegistration.OffsetY)
                }
                if (($compareX -lt 0) -or ($compareY -lt 0) -or
                    ($compareX -ge $Width) -or ($compareY -ge $Height)) {
                    continue
                }
                $other = $compareBitmap.GetPixel($compareX, $compareY)
                $delta = [Math]::Max(
                    [Math]::Abs([int]$pixel.R - [int]$other.R),
                    [Math]::Max(
                        [Math]::Abs([int]$pixel.G - [int]$other.G),
                        [Math]::Abs([int]$pixel.B - [int]$other.B)))
                if ($delta -gt 0) {
                    $rawChanged++
                }
                if ($delta -ge $CompareChannelThreshold) {
                    $changed++
                }
                $compareMaxDeltaSum += $delta
                $compareTotal++
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
    } else {
        Write-Output ("Top screen green content: {0}/{1} dominant-green pixels ({2:P3})." -f
            $dominantGreen, $total, ([double]$dominantGreen / [double]$total))
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
    } else {
        Write-Output ("Top screen detail content: {0}/{1} non-white/non-green pixels ({2:P3})." -f
            $nonWhiteNonGreen, $total,
            ([double]$nonWhiteNonGreen / [double]$total))
    }
    if ($null -ne $colorHistogram) {
        $largestColor = $colorHistogram.GetEnumerator() |
            Sort-Object Value -Descending |
            Select-Object -First 1
        $largestColorFraction = [double]$largestColor.Value / [double]$total
        if ($largestColorFraction -gt $MaxSingleColorFraction) {
            throw ("Top screen single-color concentration is too large: {0}/{1} pixels ({2:P3}) above allowed {3:P3}." -f
                $largestColor.Value, $total, $largestColorFraction,
                $MaxSingleColorFraction)
        }
        Write-Output ("Top screen largest single-color concentration: {0}/{1} pixels ({2:P3})." -f
            $largestColor.Value, $total, $largestColorFraction)
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
        for ($ry = $RequiredRegionY; $ry -lt ($RequiredRegionY + $RequiredRegionHeight); $ry++) {
            for ($rx = $RequiredRegionX; $rx -lt ($RequiredRegionX + $RequiredRegionWidth); $rx++) {
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
        if ($compareTotal -eq 0) {
            throw 'Comparison screenshot has no overlapping top-screen pixels.'
        }
        $overlapFraction = [double]$compareTotal / [double]$total
        if ($overlapFraction -lt $MinCompareOverlapFraction) {
            throw ("Top screen comparison overlap {0:P3} is below required {1:P3}." -f
                $overlapFraction, $MinCompareOverlapFraction)
        }
        $changedFraction = [double]$changed / [double]$compareTotal
        $rawChangedFraction = [double]$rawChanged / [double]$compareTotal
        $meanChannelDelta = [double]$compareMaxDeltaSum /
            [double]$compareTotal
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
        if (($MaxCompareMeanChannelDelta -ge 0.0) -and
            ($meanChannelDelta -gt $MaxCompareMeanChannelDelta)) {
            throw ("Top screen mean max-channel delta {0:F2} exceeds allowed {1:F2}." -f
                $meanChannelDelta, $MaxCompareMeanChannelDelta)
        }
        Write-Output ("Top screen delta: raw={0}/{1} ({2:P3}) meaningful={3}/{1} ({4:P3}, channel>={5}) mean={6:F2} overlap={7:P3}." -f
            $rawChanged, $compareTotal, $rawChangedFraction,
            $changed, $changedFraction, $CompareChannelThreshold,
            $meanChannelDelta, $overlapFraction)
    }
    foreach ($regionSpec in $NamedRegion) {
        $region = Convert-NamedRegionSpec $regionSpec
        Measure-TopRegion -Bitmap $bitmap -CompareBitmap $compareBitmap `
            -Name $region.Name -RegionX $region.X -RegionY $region.Y `
            -RegionWidth $region.Width -RegionHeight $region.Height
    }
} finally {
    if ($null -ne $compareBitmap) {
        $compareBitmap.Dispose()
    }
    if ($null -ne $compareWindowBitmap) {
        $compareWindowBitmap.Dispose()
    }
    if ($null -ne $bitmap) {
        $bitmap.Dispose()
    }
    $windowBitmap.Dispose()
}
