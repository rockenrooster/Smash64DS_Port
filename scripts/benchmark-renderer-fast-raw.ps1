param(
    [ValidateRange(0,8)][int]$FastRunMode = 0,
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
    [string]$RendererBenchmarkExportPath = ''
)

$ErrorActionPreference = 'Stop'
$target = if ($RendererProfileLevel -eq 2) {
    'smash64ds-battle-playable-forensic-hwtri'
} else {
    'smash64ds-battle-playable-coarse-hwtri'
}
$build = if ($RendererProfileLevel -eq 2) {
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
    -RendererFastRunMode $FastRunMode `
    -WallpaperIncrementalMode $WallpaperIncrementalMode `
    -LowerTextHudMode $LowerTextHudMode `
    -RendererBenchmarkExportPath $RendererBenchmarkExportPath `
    -Harness 'battle_playable_realtime' `
    -Target $target `
    -Build $build `
    -ExpectedMode 163 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label "battle_playable fast raw mode $FastRunMode wallpaper incremental $WallpaperIncrementalMode lower text HUD $LowerTextHudMode" `
    -HarnessSelectMessage 'Fast raw benchmark did not select Pupupu VSBattle from Maps.'
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
