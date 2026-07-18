param(
    [ValidateRange(0,9)][int]$FastRunMode = 0,
    [ValidateRange(0,1)][int]$StaticTextureAotMode = 0,
    [switch]$RequireZeroPostGoTextureFence,
    [ValidateRange(0,1)][int]$IFCommonHybridOamMode = 0,
    [ValidateRange(0,1)][int]$FoxCpuMode = 0,
    [ValidateRange(0,1)][int]$WallpaperIncrementalMode = 0,
    [ValidateRange(0,1)][int]$LowerTextHudMode = 1,
    [ValidateRange(1,2)][int]$RendererProfileLevel = 1,
    [switch]$RendererM2DetailedLedger,
    [switch]$RendererM3Phase0Profile,
    [ValidateRange(0,1)][int]$Task9FloatCensusMode = 0,
    [ValidateRange(0,1)][int]$Task9FloatItcmMode = 1,
    [ValidateRange(0,1)][int]$Task9FloatPhase2Mode = 1,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [ValidateRange(8,256)][int]$RendererBenchmarkSamples = 8,
    [ValidateRange(0,1000000)][int]$RendererBenchmarkStartFrame = 0,
    [ValidateSet('None','KO','Rebirth')]
    [string]$RendererBenchmarkStartEvent = 'None',
    [ValidateRange(5,600)][int]$RendererBenchmarkTimeoutSeconds = 30,
    [string]$RendererBenchmarkExportPath = '',
    [string]$RendererBenchmarkScreenshot = ''
)

$ErrorActionPreference = 'Stop'
if (($FastRunMode -eq 9) -and ($RendererProfileLevel -ne 1)) {
    throw 'Fast-run mode 9 requires renderer profile 1.'
}
if (($Task9FloatItcmMode -eq 0) -and
    -not $PSBoundParameters.ContainsKey('Task9FloatPhase2Mode')) {
    $Task9FloatPhase2Mode = 0
}
if (($Task9FloatPhase2Mode -eq 1) -and
    ($Task9FloatItcmMode -ne 1)) {
    throw 'Task9FloatPhase2Mode=1 requires Task9FloatItcmMode=1.'
}
if ($RendererM3Phase0Profile -and
    (($FastRunMode -ne 9) -or ($RendererProfileLevel -ne 1))) {
    throw 'RendererM3Phase0Profile requires fast-run mode 9 and renderer profile 1.'
}
if (($FastRunMode -eq 9) -and
    ($RendererBenchmarkStartEvent -eq 'None') -and
    -not $PSBoundParameters.ContainsKey('RendererBenchmarkStartFrame')) {
    # The complete-stage owner/M4 contract is meaningful only after GO and
    # static residency are armed; frames 438-445 are the canonical first gate.
    $RendererBenchmarkStartFrame = 438
}
$target = if ($FastRunMode -eq 9) {
    'smash64ds-battle-playable-m3-stage-owner-lab'
} elseif ($RendererProfileLevel -eq 2) {
    'smash64ds-battle-playable-forensic-hwtri'
} else {
    'smash64ds-battle-playable-coarse-hwtri'
}
$build = if ($Task9FloatCensusMode -eq 1) {
    'builds/build-task9-float-census-lab'
} elseif ($Task9FloatItcmMode -eq 1) {
    if ($Task9FloatPhase2Mode -eq 1) {
        'builds/build-task9-float-phase2-fcmpeq-lab'
    } else {
        'builds/build-task9-float-itcm-lab'
    }
} elseif ($RendererM3Phase0Profile) {
    'builds/build-m3-phase0-lab'
} elseif ($FastRunMode -eq 9) {
    'builds/build-m3-stage-owner-lab'
} elseif ($RendererProfileLevel -eq 2) {
    'build-battle-playable-forensic-hwtri-harness'
} else {
    'build-battle-playable-coarse-hwtri-harness'
}

& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -BattlePlayable `
    -RealtimePresentation `
    -LiveInputPreview `
    -ImportBattleShipFTComputer `
    -HardwareTriangles `
    -RendererProfileLevel $RendererProfileLevel `
    -RendererM2DetailedLedger:$RendererM2DetailedLedger `
    -RendererM3Phase0Profile:$RendererM3Phase0Profile `
    -Task9FloatCensusMode $Task9FloatCensusMode `
    -Task9FloatItcmMode $Task9FloatItcmMode `
    -Task9FloatPhase2Mode $Task9FloatPhase2Mode `
    -RendererBenchmarkSamples $RendererBenchmarkSamples `
    -RendererBenchmarkStartFrame $RendererBenchmarkStartFrame `
    -RendererBenchmarkStartEvent $RendererBenchmarkStartEvent `
    -RendererBenchmarkTimeoutSeconds $RendererBenchmarkTimeoutSeconds `
    -RendererBenchmarkOnly:($FastRunMode -eq 9) `
    -RendererBenchmarkScreenshot $RendererBenchmarkScreenshot `
    -RendererFastRunMode $FastRunMode `
    -StaticTextureAotMode $StaticTextureAotMode `
    -RequireZeroPostGoTextureFence:$RequireZeroPostGoTextureFence `
    -IFCommonHybridOamMode $IFCommonHybridOamMode `
    -FoxCpuMode $FoxCpuMode `
    -WallpaperIncrementalMode $WallpaperIncrementalMode `
    -LowerTextHudMode $LowerTextHudMode `
    -RendererBenchmarkExportPath $RendererBenchmarkExportPath `
    -Harness 'battle_playable_realtime' `
    -Target $target `
    -Build $build `
    -ExpectedMode 163 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label "battle_playable fast raw mode $FastRunMode static texture AOT $StaticTextureAotMode strict texture fence $([int]$RequireZeroPostGoTextureFence.IsPresent) frozen water $StaticTextureAotMode hybrid OAM $IFCommonHybridOamMode Fox CPU $FoxCpuMode wallpaper incremental $WallpaperIncrementalMode lower text HUD $LowerTextHudMode task9 float census/ITCM/phase2 $Task9FloatCensusMode/$Task9FloatItcmMode/$Task9FloatPhase2Mode" `
    -HarnessSelectMessage 'Fast raw benchmark did not select Pupupu VSBattle from Maps.'
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
if (-not [string]::IsNullOrWhiteSpace($RendererBenchmarkScreenshot)) {
    & (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
        -Image $RendererBenchmarkScreenshot `
        -WindowScaledCapture `
        -MinDominantGreenFraction 0.03 `
        -MinNonWhiteNonGreenFraction 0.25 `
        -MaxSingleColorFraction 0.20
}
