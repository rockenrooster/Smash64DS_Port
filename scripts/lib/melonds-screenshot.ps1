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
