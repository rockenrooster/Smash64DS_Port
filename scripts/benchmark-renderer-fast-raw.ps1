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
    [ValidateRange(0,1)][int]$NativeStageGeneratedSegment0Enable = 0,
    [switch]$Task29GXCensus,
    [switch]$Task34StageStreamCensus,
    [ValidateRange(0,2)][int]$Task36HwComposeMode = 0,
    [ValidateRange(0,1)][int]$Task44StageSteadyMode = 0,
    [switch]$Task20StackProfile,
    [ValidateRange(0,1)][int]$Task32DrawHotTextMode = 0,
    [switch]$Task22WallpaperRunLab,
    [ValidateRange(0,1)][int]$RendererScreenSpaceCensusMode = 0,
    [ValidateRange(0,1)][int]$RenderEconomyMode = 0,
    [ValidateRange(0,255)][int]$RenderEconomyOwnerMask = 0,
    [ValidateRange(0,1)][int]$Task9FloatCensusMode = 0,
    [ValidateRange(0,1)][int]$Task9FloatItcmMode = 1,
    [ValidateRange(0,1)][int]$Task9FloatPhase2Mode = 1,
    [ValidateRange(0,1)][int]$Task16FloatCompareMode = 0,
    [ValidateRange(0,1)][int]$Task16FloatI2fMode = 0,
    [ValidateRange(0,1)][int]$Task16FloatAddSubMode = 0,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [ValidateRange(8,1024)][int]$RendererBenchmarkSamples = 8,
    [ValidateRange(0,1000000)][int]$RendererBenchmarkStartFrame = 0,
    [ValidateSet('None','KO','Rebirth','Late','TimeUp')]
    [string]$RendererBenchmarkStartEvent = 'None',
    [switch]$PhaseMatrixMode,
    [ValidateRange(5,3600)][int]$RendererBenchmarkTimeoutSeconds = 30,
    [string]$RendererBenchmarkExportPath = '',
    [string]$RendererBenchmarkScreenshot = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
$selectedGdbPort = if (($RunnerSlot -ge 0) -and
    -not $PSBoundParameters.ContainsKey('GdbPort')) {
    Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
} else {
    $GdbPort
}
$nativeStageGeneratedSegment0Selected =
    $PSBoundParameters.ContainsKey('NativeStageGeneratedSegment0Enable')
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
if (($Task16FloatCompareMode -eq 1) -and
    ($Task9FloatPhase2Mode -ne 1)) {
    throw 'Task16FloatCompareMode=1 requires Task9FloatPhase2Mode=1.'
}
if (($Task16FloatI2fMode -eq 1) -and
    ($Task9FloatItcmMode -ne 1)) {
    throw 'Task16FloatI2fMode=1 requires Task9FloatItcmMode=1.'
}
if (($Task16FloatAddSubMode -eq 1) -and
    ($Task9FloatPhase2Mode -ne 1)) {
    throw 'Task16FloatAddSubMode=1 requires Task9FloatPhase2Mode=1.'
}
if ($RendererM3Phase0Profile -and
    (($FastRunMode -ne 9) -or ($RendererProfileLevel -ne 1))) {
    throw 'RendererM3Phase0Profile requires fast-run mode 9 and renderer profile 1.'
}
if (($NativeStageGeneratedSegment0Enable -eq 1) -and
    (($FastRunMode -ne 9) -or ($RendererProfileLevel -ne 1))) {
    throw 'NativeStageGeneratedSegment0Enable=1 requires fast-run mode 9 and renderer profile 1.'
}
if ($Task29GXCensus -and
    (($FastRunMode -ne 9) -or ($RendererProfileLevel -ne 1))) {
    throw 'Task29GXCensus requires fast-run mode 9 and renderer profile 1.'
}
if ($Task34StageStreamCensus -and
    (($RendererProfileLevel -ne 1) -or ($FastRunMode -ne 9))) {
    throw 'Task34StageStreamCensus requires profile 1 and complete-stage fast-run mode 9.'
}
if (($Task36HwComposeMode -gt 0) -and
    (($FastRunMode -ne 9) -or ($RendererProfileLevel -ne 1))) {
    throw 'Task36HwComposeMode requires fast-run mode 9 and renderer profile 1.'
}
if (($Task44StageSteadyMode -eq 1) -and ($Task36HwComposeMode -eq 0)) {
    throw 'Task44StageSteadyMode requires Task36HwComposeMode.'
}
if (($RendererScreenSpaceCensusMode -eq 1) -and
    (($FastRunMode -ne 9) -or ($RendererProfileLevel -ne 1))) {
    throw 'RendererScreenSpaceCensusMode requires fast-run mode 9 and renderer profile 1.'
}
if (($RenderEconomyMode -eq 1) -and
    (($FastRunMode -ne 9) -or ($RendererProfileLevel -ne 1))) {
    throw 'RenderEconomyMode requires fast-run mode 9 and renderer profile 1.'
}
if (($RenderEconomyMode -eq 0) -and ($RenderEconomyOwnerMask -ne 0)) {
    throw 'RenderEconomyOwnerMask requires RenderEconomyMode 1.'
}
if ($Task20StackProfile -and ($RendererProfileLevel -ne 1)) {
    throw 'Task20StackProfile requires renderer profile 1.'
}
if ($Task20StackProfile -and $PhaseMatrixMode) {
    throw 'Task20StackProfile is a separate startup stack census and cannot contaminate the phase matrix.'
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
$build = if ($PSBoundParameters.ContainsKey('Task44StageSteadyMode')) {
    "builds/build-task44-stage-steady-e${Task44StageSteadyMode}-lab"
} elseif ($PSBoundParameters.ContainsKey('Task36HwComposeMode')) {
    "builds/build-task36-hw-compose-e${Task36HwComposeMode}-lab"
} elseif ($PSBoundParameters.ContainsKey('Task32DrawHotTextMode')) {
    "builds/build-task32-draw-hot-e${Task32DrawHotTextMode}-lab"
} elseif ($Task20StackProfile) {
    'builds/build-task20-reconcile'
} elseif ($Task22WallpaperRunLab) {
    "builds/build-task22-wallpaper-profile${RendererProfileLevel}-lab"
} elseif ($Task34StageStreamCensus) {
    'builds/build-task34-stage-stream-census-lab'
} elseif ($Task29GXCensus) {
    'builds/build-task29-gx-census-lab'
} elseif ($nativeStageGeneratedSegment0Selected) {
    "builds/build-task26-segment0-e${NativeStageGeneratedSegment0Enable}-p$([int]$RendererM3Phase0Profile.IsPresent)-lab"
} elseif ($RendererScreenSpaceCensusMode -eq 1) {
    'builds/build-task11-screen-space-census-lab'
} elseif ($RenderEconomyMode -eq 1) {
    'builds/build-task11-economy-lab'
} elseif ($Task9FloatCensusMode -eq 1) {
    'builds/build-task9-float-census-lab'
} elseif (($Task16FloatCompareMode -eq 1) -or
          ($Task16FloatI2fMode -eq 1) -or
          ($Task16FloatAddSubMode -eq 1)) {
    "builds/build-task16-float-c${Task16FloatCompareMode}-i${Task16FloatI2fMode}-a${Task16FloatAddSubMode}-lab"
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
    -GdbPort $selectedGdbPort `
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
    -NativeStageGeneratedSegment0Enable $NativeStageGeneratedSegment0Enable `
    -Task29GXCensus:$Task29GXCensus `
    -Task34StageStreamCensus:$Task34StageStreamCensus `
    -Task36HwComposeMode $Task36HwComposeMode `
    -Task44StageSteadyMode $Task44StageSteadyMode `
    -Task20StackProfileMode ([int]$Task20StackProfile.IsPresent) `
    -Task32DrawHotTextMode $Task32DrawHotTextMode `
    -Task22WallpaperRunLab:$Task22WallpaperRunLab `
    -RendererScreenSpaceCensusMode $RendererScreenSpaceCensusMode `
    -RenderEconomyMode $RenderEconomyMode `
    -RenderEconomyOwnerMask $RenderEconomyOwnerMask `
    -Task9FloatCensusMode $Task9FloatCensusMode `
    -Task9FloatItcmMode $Task9FloatItcmMode `
    -Task9FloatPhase2Mode $Task9FloatPhase2Mode `
    -Task16FloatCompareMode $Task16FloatCompareMode `
    -Task16FloatI2fMode $Task16FloatI2fMode `
    -Task16FloatAddSubMode $Task16FloatAddSubMode `
    -RendererBenchmarkSamples $RendererBenchmarkSamples `
    -RendererBenchmarkStartFrame $RendererBenchmarkStartFrame `
    -RendererBenchmarkStartEvent $RendererBenchmarkStartEvent `
    -PhaseMatrixMode:$PhaseMatrixMode `
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
    -Label "battle_playable fast raw mode $FastRunMode generated segment0 $NativeStageGeneratedSegment0Enable task29 GX census $([int]$Task29GXCensus.IsPresent) task34 stage stream $([int]$Task34StageStreamCensus.IsPresent) task36 HW compose $Task36HwComposeMode static texture AOT $StaticTextureAotMode strict texture fence $([int]$RequireZeroPostGoTextureFence.IsPresent) frozen water $StaticTextureAotMode hybrid OAM $IFCommonHybridOamMode Fox CPU $FoxCpuMode wallpaper incremental $WallpaperIncrementalMode task20 startup stack census $([int]$Task20StackProfile.IsPresent) task32 draw hot text $Task32DrawHotTextMode task22 run census $([int]$Task22WallpaperRunLab.IsPresent) phase matrix $([int]$PhaseMatrixMode.IsPresent) lower text HUD $LowerTextHudMode screen census $RendererScreenSpaceCensusMode economy $RenderEconomyMode/$RenderEconomyOwnerMask task9 float census/ITCM/phase2 $Task9FloatCensusMode/$Task9FloatItcmMode/$Task9FloatPhase2Mode task16 compare/i2f/addsub $Task16FloatCompareMode/$Task16FloatI2fMode/$Task16FloatAddSubMode" `
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
