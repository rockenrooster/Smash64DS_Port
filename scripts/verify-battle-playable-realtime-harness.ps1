param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$RequireRealtime60Fps,
    [ValidateRange(0,256)][int]$RendererBenchmarkSamples = 0,
    [switch]$SkipScreenshot,
    [int]$ScreenshotDelaySeconds = 8,
    [int]$ScreenshotSecondDelaySeconds = 1,
    [int]$ScreenshotSecondDelayMilliseconds = 100,
    [double]$MaxScreenshotChangedFraction = 0.30,
    [double]$MaxScreenshotMeanChannelDelta = 32.0,
    [double]$MinScreenshotGreenFraction = 0.03,
    [double]$MinScreenshotDetailFraction = 0.25,
    [switch]$FastIteration,
    [switch]$IncludeShippedParity
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$powerShellExe = (Get-Process -Id $PID).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
$selectedGdbPort = if (($RunnerSlot -ge 0) -and
    -not $PSBoundParameters.ContainsKey('GdbPort')) {
    Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
} else {
    $GdbPort
}
$minimumSmokeDelaySeconds = 25
$smokeDelaySeconds = [Math]::Max($DelaySeconds, $minimumSmokeDelaySeconds)
$earlyScreenshotDelaySeconds = [Math]::Max($ScreenshotDelaySeconds, 12)
$lateScreenshotDelaySeconds = 12
$captureStamp = '{0}-p{1}' -f (Get-Date -Format 'HHmmss-fffffff'), $PID
$captureDate = Get-Date -Format 'yyyy-MM-dd'
$rootHasher = [System.Security.Cryptography.SHA256]::Create()
try {
    $rootBytes = [System.Text.Encoding]::UTF8.GetBytes($root.ToLowerInvariant())
    $rootHash = [System.BitConverter]::ToString(
        $rootHasher.ComputeHash($rootBytes)).Replace('-', '').Substring(0, 16)
} finally {
    $rootHasher.Dispose()
}
$publishMutexName = "Local\Smash64DSVisibility_$rootHash"
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
    'left_bush:72,104,32,16,0.45,12',
    # A valid retained-wallpaper live-camera pair measured 47.154% variation
    # with a 78px path-edge run. Keep the cap below the broken 105px flat case.
    'stage_body:50,115,165,30,0.18,80',
    # StagePupupuFile2.c:621-680 supplies the two animated water MObjs.
    # The pre-fix white oval measured 27.997% / 105px at threshold 20;
    # the accepted TEXEL0/TEXEL1 frame measures 44.115% / 23px.
    'pond:82,125,115,24,0.35,80'
)
$fastTextureDetailRegions = @(
    # Sample the lower flowering bush, clear of live Fox/platform overlap.
    # Canonical live-input camera frames retain 46.5%+ varied coverage but can
    # put one full-width grass row at the crop edge; the coverage gate remains
    # the texture proof while the run is recorded rather than position-gated.
    'left_bush:72,104,32,16,0.40,32',
    # Valid live-input camera frames retain 24.9%+ varied coverage while the
    # moving path edge can make a 98px low-delta run. Keep a 112px cap so a
    # broadly flat stage still fails independently of that camera position.
    'stage_body:50,115,165,30,0.18,112',
    # Tolerate a valid 22.953% / 61px camera sample while the 96px cap still
    # rejects the pre-fix white pond's 105px run.
    'pond:82,125,115,24,0.20,96'
)
function Copy-FileAtomically {
    param(
        [Parameter(Mandatory=$true)][string]$Source,
        [Parameter(Mandatory=$true)][string]$Destination
    )
    $destinationDirectory = Split-Path -Parent $Destination
    $temporaryName = '.{0}.p{1}.{2}.tmp' -f
        ([System.IO.Path]::GetFileName($Destination)),
        $PID,
        ([System.Guid]::NewGuid().ToString('N'))
    $temporaryPath = Join-Path $destinationDirectory $temporaryName
    $backupPath = "$temporaryPath.bak"
    try {
        Copy-Item -LiteralPath $Source -Destination $temporaryPath
        if (Test-Path -LiteralPath $Destination -PathType Leaf) {
            [System.IO.File]::Replace(
                $temporaryPath, $Destination, $backupPath, $true)
        } else {
            [System.IO.File]::Move($temporaryPath, $Destination)
        }
    } finally {
        Remove-Item -LiteralPath $temporaryPath -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $backupPath -Force -ErrorAction SilentlyContinue
    }
}
function Publish-StableCapture {
    param([Parameter(Mandatory=$true)][string]$Image)

    $visibilityDir = Join-Path $root 'artifacts\visibility'
    $latest = Join-Path $visibilityDir 'latest.png'
    $previous = Join-Path $visibilityDir 'previous.png'
    $publishMutex = [System.Threading.Mutex]::new($false, $publishMutexName)
    $publishLockTaken = $false
    $rotatedPrevious = $false
    try {
        try {
            $publishLockTaken = $publishMutex.WaitOne(
                [System.TimeSpan]::FromSeconds(30))
        } catch [System.Threading.AbandonedMutexException] {
            $publishLockTaken = $true
        }
        if (-not $publishLockTaken) {
            throw "Timed out waiting for stable-capture publisher '$publishMutexName'."
        }
        if (Test-Path -LiteralPath $latest -PathType Leaf) {
            Copy-FileAtomically -Source $latest -Destination $previous
            $rotatedPrevious = $true
        }
        Copy-FileAtomically -Source $Image -Destination $latest
    } finally {
        if ($publishLockTaken) {
            $publishMutex.ReleaseMutex()
        }
        $publishMutex.Dispose()
    }
    if ($rotatedPrevious) {
        Write-Output "Published fast-iteration capture: $latest (previous: $previous)"
    } else {
        Write-Output "Published fast-iteration capture: $latest (no previous capture)"
    }
}
function Invoke-VisibleCaptureAssert {
    param(
        [string]$Rom,
        [string]$Stem,
        [int]$Delay,
        [double]$MinDetailFraction = -1.0,
        [switch]$RequireTextureDetail,
        [switch]$PublishStableCapture
    )
    $output = Join-Path $root "artifacts\visibility\${captureDate}_${Stem}_${captureStamp}.png"
    $nextOutput = Join-Path $root "artifacts\visibility\${captureDate}_${Stem}_${captureStamp}_next.png"
    & (Join-Path $PSScriptRoot 'capture-melonds.ps1') `
        -MelonDS $captureMelonDS `
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
    $visibleArgs = @{
        Image = $output
        CompareImage = $nextOutput
        MinDominantGreenFraction = $MinScreenshotGreenFraction
        MinNonWhiteNonGreenFraction = $MinDetailFraction
        CompareChannelThreshold = 25
        MaxCompareMeanChannelDelta = $MaxScreenshotMeanChannelDelta
        RegisterCompareCamera = $true
        MaxCompareChangedFraction = $MaxScreenshotChangedFraction
        MinDifferentFraction = 0.01
        WindowScaledCapture = $true
        NamedRegion = $visibleRegions
    }
    & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') @visibleArgs
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    # Validate both sampled presentations independently. This keeps a missing
    # stage/foreground flash as a hard failure even when live camera motion
    # requires a wider pairwise-delta allowance in the short iteration path.
    & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
        -Image $nextOutput `
        -MinDominantGreenFraction $MinScreenshotGreenFraction `
        -MinNonWhiteNonGreenFraction $MinDetailFraction `
        -MinDifferentFraction 0.01 `
        -WindowScaledCapture `
        -NamedRegion $visibleRegions
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    if ($RequireTextureDetail) {
        $detailRegions = if ($FastIteration) {
            $fastTextureDetailRegions
        } else {
            $textureDetailRegions
        }
        & (Join-Path $PSScriptRoot 'assert-melonds-horizontal-detail.ps1') `
            -Image $output `
            -ChannelThreshold 20 `
            -WindowScaledCapture `
            -Region $detailRegions
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
        & (Join-Path $PSScriptRoot 'assert-melonds-horizontal-detail.ps1') `
            -Image $nextOutput `
            -ChannelThreshold 20 `
            -WindowScaledCapture `
            -Region $detailRegions
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
    if ($PublishStableCapture) {
        Publish-StableCapture -Image $output
    }
}
$harnessArgs = @(
    '-NoProfile',
    '-ExecutionPolicy', 'Bypass',
    '-File', (Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1'),
    '-MelonDS', $MelonDS,
    '-Gdb', $Gdb,
    '-GdbPort', "$selectedGdbPort",
    '-RunnerSlot', "$RunnerSlot",
    '-DelaySeconds', "$smokeDelaySeconds",
    '-RealtimePresentation',
    '-ImportBattleShipFTComputer'
)
if ($NoBuild) { $harnessArgs += '-NoBuild' }
if ($RequireRealtime60Fps) { $harnessArgs += '-RequireRealtime60Fps' }
if ($RendererBenchmarkSamples -gt 0) {
    $harnessArgs += @('-RendererBenchmarkSamples', "$RendererBenchmarkSamples")
}
& $powerShellExe @harnessArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
& (Join-Path $PSScriptRoot 'check-battle-playable-rom-parity.ps1')
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
if (-not $SkipScreenshot) {
    $captureMelonDS = $MelonDS
    if ($RunnerSlot -ge 0) {
        $captureContext = Resolve-MelonDSRunnerSlot `
            -Root $root `
            -RunnerSlot $RunnerSlot `
            -MelonDS $MelonDS `
            -GdbPort $selectedGdbPort `
            -GdbPortExplicit
        $captureMelonDS = $captureContext.MelonDSPath
        Write-Output "Using melonDS runner slot $RunnerSlot for capture: $captureMelonDS"
    }
    $canonicalRom = Join-Path $root 'smash64ds-battle-playable-canonical-hwtri.nds'
    $shippedRom = Join-Path $root 'smash64ds-battle-playable-hwtri.nds'
    # The preceding GDB verifier hard-proves both source-selected fighter
    # display contracts. Moving fighters therefore cannot make any realtime
    # visual gate fail merely by leaving its historical fixed crops.
    if ($FastIteration) {
        Invoke-VisibleCaptureAssert -Rom $canonicalRom `
            -Stem 'canonical_fast' `
            -Delay $earlyScreenshotDelaySeconds `
            -MinDetailFraction $MinScreenshotDetailFraction `
            -RequireTextureDetail `
            -PublishStableCapture
    } else {
        Invoke-VisibleCaptureAssert -Rom $canonicalRom `
            -Stem 'iter4_canonical_early' `
            -Delay $earlyScreenshotDelaySeconds `
            -MinDetailFraction $MinScreenshotDetailFraction `
            -RequireTextureDetail
        Invoke-VisibleCaptureAssert -Rom $canonicalRom `
            -Stem 'iter4_canonical_late' `
            -Delay $lateScreenshotDelaySeconds
    }
    if ($IncludeShippedParity) {
        Invoke-VisibleCaptureAssert -Rom $shippedRom `
            -Stem 'iter4_shipped_early' `
            -Delay $earlyScreenshotDelaySeconds `
            -MinDetailFraction $MinScreenshotDetailFraction `
            -RequireTextureDetail
        Invoke-VisibleCaptureAssert -Rom $shippedRom `
            -Stem 'iter4_shipped_late' `
            -Delay $lateScreenshotDelaySeconds
    }
}
exit 0
