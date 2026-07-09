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
    [int]$ScreenshotSecondDelayMilliseconds = 100,
    [double]$MaxScreenshotChangedFraction = 0.25,
    [double]$MinScreenshotGreenFraction = 0.03,
    [double]$MinScreenshotDetailFraction = 0.25,
    [double]$MinFighterRegionFraction = 0.02
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$powerShellExe = (Get-Process -Id $PID).Path
$smokeDelaySeconds = [Math]::Max($DelaySeconds, 20)
$earlyScreenshotDelaySeconds = [Math]::Max($ScreenshotDelaySeconds, 8)
$lateScreenshotDelaySeconds = 30
function Invoke-VisibleCaptureAssert {
    param(
        [string]$Rom,
        [string]$Stem,
        [int]$Delay
    )
    $output = Join-Path $root "artifacts\visibility\2026-07-09_${Stem}.png"
    $nextOutput = Join-Path $root "artifacts\visibility\2026-07-09_${Stem}_next.png"
    & (Join-Path $PSScriptRoot 'capture-melonds.ps1') `
        -MelonDS $MelonDS `
        -Rom $Rom `
        -Output $output `
        -SecondOutput $nextOutput `
        -SecondDelaySeconds $ScreenshotSecondDelaySeconds `
        -SecondDelayMilliseconds $ScreenshotSecondDelayMilliseconds `
        -DelaySeconds $Delay
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
        -Image $output `
        -CompareImage $nextOutput `
        -MinDominantGreenFraction $MinScreenshotGreenFraction `
        -MinNonWhiteNonGreenFraction $MinScreenshotDetailFraction `
        -RequiredRegionX 92 `
        -RequiredRegionY 70 `
        -RequiredRegionWidth 78 `
        -RequiredRegionHeight 72 `
        -MinRequiredRegionFraction 0.05 `
        -MinRequiredRegionFighterFraction $MinFighterRegionFraction `
        -MaxCompareChangedFraction $MaxScreenshotChangedFraction `
        -MinDifferentFraction 0.01
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
$harnessArgs = @(
    '-NoProfile',
    '-ExecutionPolicy', 'Bypass',
    '-File', (Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1'),
    '-MelonDS', $MelonDS,
    '-Gdb', $Gdb,
    '-GdbPort', "$GdbPort",
    '-RunnerSlot', "$RunnerSlot",
    '-DelaySeconds', "$smokeDelaySeconds",
    '-RealtimePresentation'
)
if ($NoBuild) { $harnessArgs += '-NoBuild' }
if ($RequireRealtime60Fps) { $harnessArgs += '-RequireRealtime60Fps' }
& $powerShellExe @harnessArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
if (-not $SkipScreenshot) {
    $canonicalRom = Join-Path $root 'smash64ds-battle-playable-canonical-hwtri.nds'
    $shippedRom = Join-Path $root 'smash64ds-battle-playable-hwtri.nds'
    Invoke-VisibleCaptureAssert -Rom $canonicalRom -Stem 'canonical_parity_early' -Delay $earlyScreenshotDelaySeconds
    Invoke-VisibleCaptureAssert -Rom $canonicalRom -Stem 'canonical_parity_late' -Delay $lateScreenshotDelaySeconds
    Invoke-VisibleCaptureAssert -Rom $shippedRom -Stem 'shipped_parity_early' -Delay $earlyScreenshotDelaySeconds
    Invoke-VisibleCaptureAssert -Rom $shippedRom -Stem 'shipped_parity_late' -Delay $lateScreenshotDelaySeconds
}
exit 0
