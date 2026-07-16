param(
    [ValidateRange(0,9)][int]$FastRunMode = 0,
    [ValidateRange(0,1)][int]$StaticTextureAotMode = 0,
    [switch]$RequireZeroPostGoTextureFence,
    [ValidateRange(0,1)][int]$M4WaterTiledAotMode = 0,
    [ValidateRange(0,1)][int]$IFCommonHybridOamMode = 0,
    [ValidateRange(0,1)][int]$FoxCpuMode = 0,
    [ValidateRange(0,1)][int]$WallpaperIncrementalMode = 0,
    [ValidateRange(0,1)][int]$LowerTextHudMode = 1,
    [ValidateRange(1,2)][int]$RendererProfileLevel = 1,
    [switch]$RendererM2DetailedLedger,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [ValidateRange(8,256)][int]$RendererBenchmarkSamples = 32,
    [ValidateRange(0,1000000)][int]$RendererBenchmarkStartFrame = 0,
    [ValidateRange(5,600)][int]$RendererBenchmarkTimeoutSeconds = 30,
    [string]$RendererBenchmarkExportPath = '',
    [string]$RendererBenchmarkScreenshot = ''
)

$ErrorActionPreference = 'Stop'
if (($FastRunMode -eq 9) -and ($RendererProfileLevel -ne 1)) {
    throw 'Fast-run mode 9 requires renderer profile 1.'
}
if ($M4WaterTiledAotMode -ne 0) {
    throw 'M4WaterTiledAotMode is retired; frozen source water is part of StaticTextureAotMode 1.'
}
if (($FastRunMode -eq 9) -and
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
$build = if ($FastRunMode -eq 9) {
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
    -RendererBenchmarkSamples $RendererBenchmarkSamples `
    -RendererBenchmarkStartFrame $RendererBenchmarkStartFrame `
    -RendererBenchmarkTimeoutSeconds $RendererBenchmarkTimeoutSeconds `
    -RendererBenchmarkOnly:($FastRunMode -eq 9) `
    -RendererBenchmarkScreenshot $RendererBenchmarkScreenshot `
    -RendererFastRunMode $FastRunMode `
    -StaticTextureAotMode $StaticTextureAotMode `
    -RequireZeroPostGoTextureFence:$RequireZeroPostGoTextureFence `
    -M4WaterTiledAotMode $M4WaterTiledAotMode `
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
    -Label "battle_playable fast raw mode $FastRunMode static texture AOT $StaticTextureAotMode strict texture fence $([int]$RequireZeroPostGoTextureFence.IsPresent) frozen water $StaticTextureAotMode hybrid OAM $IFCommonHybridOamMode Fox CPU $FoxCpuMode wallpaper incremental $WallpaperIncrementalMode lower text HUD $LowerTextHudMode" `
    -HarnessSelectMessage 'Fast raw benchmark did not select Pupupu VSBattle from Maps.'
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
