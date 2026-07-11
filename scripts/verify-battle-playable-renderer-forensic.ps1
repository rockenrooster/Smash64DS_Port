param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [ValidateRange(0,16)][int]$RendererBenchmarkSamples = 0
)
$ErrorActionPreference = 'Stop'

& (Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -RealtimePresentation `
    -ImportBattleShipFTComputer `
    -RendererProfileLevel 2 `
    -RendererBenchmarkSamples $RendererBenchmarkSamples
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Output 'Forensic canonical renderer oracle passed at profile level 2.'
exit 0
