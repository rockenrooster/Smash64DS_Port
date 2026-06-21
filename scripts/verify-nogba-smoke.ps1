param(
    [switch]$Build,
    [string]$NoGba = (Join-Path $PSScriptRoot '..\emulators\nogba\NO$GBA.EXE'),
    [int]$DelaySeconds = 5
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$output = Join-Path $root "artifacts\nogba-smoke-$stamp.png"
$outputDirectory = Split-Path -Parent $output
$outputStem = [System.IO.Path]::GetFileNameWithoutExtension($output)

& (Join-Path $PSScriptRoot 'capture-nogba.ps1') `
    -Build:$Build `
    -NoGba $NoGba `
    -Output $output `
    -DelaySeconds $DelaySeconds `
    -AllWindows

$captures = @(
    Get-ChildItem -Path $outputDirectory -Filter "$outputStem-w*.png" `
        -File -ErrorAction SilentlyContinue |
        Sort-Object Name
)
if ($captures.Count -eq 0 -and (Test-Path $output)) {
    $captures = @(Get-Item $output)
}
if ($captures.Count -eq 0) {
    throw "no`$gba smoke capture was not created: $output"
}

Add-Type -AssemblyName System.Drawing
$validCount = 0
foreach ($capture in $captures) {
    $bitmap = [System.Drawing.Bitmap]::FromFile($capture.FullName)
    try {
        if (($bitmap.Width -lt 128) -or ($bitmap.Height -lt 128)) {
            throw "no`$gba smoke capture is unexpectedly small: $($bitmap.Width)x$($bitmap.Height) ($($capture.FullName))"
        }
        $validCount++
    } finally {
        $bitmap.Dispose()
    }
}

Write-Output "no`$gba smoke verification passed: $validCount capture(s), stem $outputStem"
