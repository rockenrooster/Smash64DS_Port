param(
    [Parameter(Mandatory=$true)]
    [string]$Image,
    [int]$TopX = 8,
    [int]$TopY = 56,
    [int]$Width = 256,
    [int]$Height = 192,
    [int]$ClearR = 20,
    [int]$ClearG = 28,
    [int]$ClearB = 52,
    [double]$MinDifferentFraction = 0.01
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path $Image)) {
    throw "Screenshot not found: $Image"
}
Add-Type -AssemblyName System.Drawing
$bitmap = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Image).Path)
try {
    if (($TopX + $Width) -gt $bitmap.Width -or
        ($TopY + $Height) -gt $bitmap.Height) {
        throw "Top-screen crop ${TopX},${TopY} ${Width}x${Height} exceeds image $($bitmap.Width)x$($bitmap.Height)."
    }
    $different = 0
    $total = $Width * $Height
    for ($y = $TopY; $y -lt ($TopY + $Height); $y++) {
        for ($x = $TopX; $x -lt ($TopX + $Width); $x++) {
            $pixel = $bitmap.GetPixel($x, $y)
            if (($pixel.R -ne $ClearR) -or
                ($pixel.G -ne $ClearG) -or
                ($pixel.B -ne $ClearB)) {
                $different++
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
} finally {
    $bitmap.Dispose()
}
