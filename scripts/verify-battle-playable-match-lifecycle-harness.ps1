param(
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 3
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1') `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -ImportBattleShipFTComputer `
    -CPUOpponentProof `
    -MatchLifecycleProof
exit $LASTEXITCODE
