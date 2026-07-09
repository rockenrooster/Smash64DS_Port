param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$RequireRealtime60Fps,
    [switch]$SkipScreenshot,
    [int]$ScreenshotDelaySeconds = 8,
    [int]$ScreenshotSecondDelaySeconds = 1,
    [double]$MaxScreenshotChangedFraction = 0.25,
    [double]$MinScreenshotGreenFraction = 0.03
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$powerShellExe = (Get-Process -Id $PID).Path
$harnessArgs = @(
    '-NoProfile',
    '-ExecutionPolicy', 'Bypass',
    '-File', (Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1'),
    '-MelonDS', $MelonDS,
    '-Gdb', $Gdb,
    '-GdbPort', "$GdbPort",
    '-RunnerSlot', "$RunnerSlot",
    '-DelaySeconds', "$DelaySeconds",
    '-RealtimePresentation'
)
if ($NoBuild) { $harnessArgs += '-NoBuild' }
if ($RequireRealtime60Fps) { $harnessArgs += '-RequireRealtime60Fps' }
& $powerShellExe @harnessArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
if (-not $SkipScreenshot) {
    $rom = Join-Path $root 'smash64ds-battle-playable-canonical-hwtri.nds'
    $output = Join-Path $root 'artifacts\visibility\canonical-hwtri-verified.png'
    $secondOutput = Join-Path $root 'artifacts\visibility\canonical-hwtri-verified-next.png'
    & (Join-Path $PSScriptRoot 'capture-melonds.ps1') `
        -MelonDS $MelonDS `
        -Rom $rom `
        -Output $output `
        -SecondOutput $secondOutput `
        -SecondDelaySeconds $ScreenshotSecondDelaySeconds `
        -DelaySeconds $ScreenshotDelaySeconds
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
        -Image $output `
        -CompareImage $secondOutput `
        -MinDominantGreenFraction $MinScreenshotGreenFraction `
        -MaxCompareChangedFraction $MaxScreenshotChangedFraction `
        -MinDifferentFraction 0.01
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
exit 0
