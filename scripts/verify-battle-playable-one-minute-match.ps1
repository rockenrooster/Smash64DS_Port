param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 0
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')

$selectedGdbPort = if (($RunnerSlot -ge 0) -and
    -not $PSBoundParameters.ContainsKey('GdbPort')) {
    Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
} else {
    $GdbPort
}

# This release gate runs the canonical one-minute BattleShip timer from its
# exact locked Wait state through Time Up and imported VS Results. Realtime
# pacing and renderer profiling are disabled, while the hardware renderer keeps
# the published 9/0/1 plus hybrid-OAM residency configuration. This remains a
# state/lifecycle/safety/reserve gate, not a realtime or 60-FPS result.
Write-Output 'Starting mode-163 one-minute match (unthrottled state/memory only; realtime performance is not measured).'
& (Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $selectedGdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -ImportBattleShipFTComputer `
    -CPUOpponentProof `
    -MatchLifecycleProof `
    -OneMinuteMatchProof `
    -RendererFastRunMode 9 `
    -StaticTextureAotMode 1 `
    -IFCommonHybridOamMode 1 `
    -RequireZeroPostGoTextureFence
exit $LASTEXITCODE
