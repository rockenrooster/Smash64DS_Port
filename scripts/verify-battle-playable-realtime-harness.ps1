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
    [double]$MinFighterRegionFraction = 0.10,
    [switch]$IncludeShippedParity
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$powerShellExe = (Get-Process -Id $PID).Path
$smokeDelaySeconds = [Math]::Max($DelaySeconds, 20)
$earlyScreenshotDelaySeconds = [Math]::Max($ScreenshotDelaySeconds, 12)
$lateScreenshotDelaySeconds = 12
$captureStamp = Get-Date -Format 'HHmmss'
$visibleRegions = @(
    'left_bush:70,82,50,35',
    'right_bush:165,82,50,35',
    'fox:80,55,45,55',
    'mario:125,85,45,55',
    'stage_body:50,110,165,45'
)
$textureDetailRegions = @(
    # StagePupupuFile2.c:423-424 and StagePupupuImages.c:103-113 place the
    # flowering side object below the platform; keep this texture gate on it.
    'right_bush:170,88,40,20,0.50,16',
    'stage_body:50,115,165,30,0.30,0'
)
function Invoke-VisibleCaptureAssert {
    param(
        [string]$Rom,
        [string]$Stem,
        [int]$Delay,
        [double]$MinDetailFraction = -1.0,
        [double]$MinRegionFraction = -1.0,
        [double]$MinRegionFighterFraction = -1.0,
        [switch]$RequireTextureDetail
    )
    $output = Join-Path $root "artifacts\visibility\2026-07-09_${Stem}_${captureStamp}.png"
    $nextOutput = Join-Path $root "artifacts\visibility\2026-07-09_${Stem}_${captureStamp}_next.png"
    & (Join-Path $PSScriptRoot 'capture-melonds.ps1') `
        -MelonDS $MelonDS `
        -Rom $Rom `
        -SoftwareRenderer `
        -MaximizeVertical `
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
        -MinNonWhiteNonGreenFraction $MinDetailFraction `
        -RequiredRegionX 80 `
        -RequiredRegionY 55 `
        -RequiredRegionWidth 45 `
        -RequiredRegionHeight 55 `
        -MinRequiredRegionFraction $MinRegionFraction `
        -MinRequiredRegionFighterFraction $MinRegionFighterFraction `
        -CompareChannelThreshold 25 `
        -MaxCompareMeanChannelDelta 32 `
        -RegisterCompareCamera `
        -MaxCompareChangedFraction $MaxScreenshotChangedFraction `
        -MinDifferentFraction 0.01 `
        -WindowScaledCapture `
        -NamedRegion $visibleRegions
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    if ($MinRegionFighterFraction -ge 0.0) {
        & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
            -Image $output `
            -RequiredRegionX 125 `
            -RequiredRegionY 85 `
            -RequiredRegionWidth 45 `
            -RequiredRegionHeight 55 `
            -MinRequiredRegionFraction $MinRegionFraction `
            -MinRequiredRegionFighterFraction $MinRegionFighterFraction `
            -MinDifferentFraction 0.01 `
            -WindowScaledCapture
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
    if ($RequireTextureDetail) {
        & (Join-Path $PSScriptRoot 'assert-melonds-horizontal-detail.ps1') `
            -Image $output `
            -ChannelThreshold 20 `
            -WindowScaledCapture `
            -Region $textureDetailRegions
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
    # scvsbattle.c:152-159 creates the wallpaper camera/object path;
    # grwallpaper.c:45-123,132-159 keeps this sky Sprite covering the viewport.
    & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
        -Image $output `
        -RequiredRegionX 0 `
        -RequiredRegionY 0 `
        -RequiredRegionWidth 80 `
        -RequiredRegionHeight 90 `
        -MinRequiredRegionFraction 0.50
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
    '-RealtimePresentation',
    '-ImportBattleShipFTComputer'
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
    Invoke-VisibleCaptureAssert -Rom $canonicalRom `
        -Stem 'iter4_canonical_early' `
        -Delay $earlyScreenshotDelaySeconds `
        -MinDetailFraction $MinScreenshotDetailFraction `
        -MinRegionFraction 0.05 `
        -MinRegionFighterFraction $MinFighterRegionFraction `
        -RequireTextureDetail
    Invoke-VisibleCaptureAssert -Rom $canonicalRom `
        -Stem 'iter4_canonical_late' `
        -Delay $lateScreenshotDelaySeconds
    if ($IncludeShippedParity) {
        Invoke-VisibleCaptureAssert -Rom $shippedRom `
            -Stem 'iter4_shipped_early' `
            -Delay $earlyScreenshotDelaySeconds `
            -MinDetailFraction $MinScreenshotDetailFraction `
            -MinRegionFraction 0.05 `
            -MinRegionFighterFraction $MinFighterRegionFraction `
            -RequireTextureDetail
        Invoke-VisibleCaptureAssert -Rom $shippedRom `
            -Stem 'iter4_shipped_late' `
            -Delay $lateScreenshotDelaySeconds
    }
}
exit 0
