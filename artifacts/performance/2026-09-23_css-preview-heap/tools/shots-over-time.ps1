param(
    [Parameter(Mandatory = $true)][string]$Build,
    [Parameter(Mandatory = $true)][string]$Tag,
    [string]$Target = 'smash64ds-p2-shell-freeplay-hwtri',
    [int]$RunnerSlot = 7,
    [int[]]$Seconds = @(8, 14, 20, 26, 32, 38, 44, 50, 56, 62)
)
# Boot a ROM unattended (its own menu walk drives the screens) and capture
# the emulator window at fixed wall-clock offsets.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
. (Join-Path $root 'scripts\lib\melonds.ps1')
. (Join-Path $root 'scripts\lib\build-output.ps1')
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS (Join-Path $root 'emulators\melonds\melonDS.exe') `
    -RunnerSlot $RunnerSlot -NoBuild:$true
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$out = Join-Path $root "artifacts\visibility\2026-09-23_css-bisect\$Tag"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$emulator = $null
try {
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) -PassThru
    $start = Get-Date
    foreach ($s in $Seconds) {
        $wait = $s - ((Get-Date) - $start).TotalSeconds
        if ($wait -gt 0) { Start-Sleep -Milliseconds ([int]($wait * 1000)) }
        $png = Join-Path $out ('t{0:d3}.png' -f $s)
        & (Join-Path $root 'scripts\capture-running-melonds-window.ps1') `
            -EmulatorProcessId $emulator.Id -Output $png | Out-Null
    }
    Get-ChildItem $out -Filter *.png | ForEach-Object { $_.Name }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
}
