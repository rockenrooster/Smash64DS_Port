param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [ValidateRange(32,256)][int]$RendererBenchmarkSamples = 128
)

$ErrorActionPreference = 'Stop'

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
    -RendererProfileLevel 1 `
    -RendererBenchmarkSamples $RendererBenchmarkSamples `
    -RendererBenchmarkOnly `
    -Harness 'battle_playable_realtime' `
    -Target 'smash64ds-battle-playable-coarse-triangle-noop-hwtri' `
    -Build 'build-battle-playable-coarse-triangle-noop-hwtri-harness' `
    -ExpectedMode 163 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'battle_playable TRIANGLE_NOOP floor' `
    -HarnessSelectMessage 'TRIANGLE_NOOP benchmark did not select Pupupu VSBattle from Maps.'
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
