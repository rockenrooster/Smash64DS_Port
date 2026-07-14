param(
    [int]$RunnerSlot = -1,
    [int]$GdbPort = 4333,
    [switch]$NoBuild,
    [int]$DelaySeconds = 3
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1') `
    -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -ImportBattleShipFTComputer `
    -CPUOpponentProof `
    -MatchLifecycleProof
exit $LASTEXITCODE
