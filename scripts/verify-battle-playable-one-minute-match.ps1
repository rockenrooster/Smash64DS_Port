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
# exact locked Wait state through Time Up and imported VS Results. Every
# presented frame owns exactly two unchanged source updates; slower phases are
# reported rather than repaid with catch-up updates.
Write-Output 'Starting mode-163 one-minute match (locked-30, exactly two source updates per presented frame).'
& (Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $selectedGdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -ImportBattleShipFTComputer `
    -RealtimePresentation `
    -CPUOpponentProof `
    -MatchLifecycleProof `
    -OneMinuteMatchProof `
    -RequireLocked30Pacing `
    -RendererFastRunMode 9 `
    -NativeStageGeneratedSegment0Enable 1 `
    -Task36HwComposeMode 2 `
    -StaticTextureAotMode 1 `
    -IFCommonHybridOamMode 0 `
    -FastWallpaperAffineMode 1 `
    -RequireZeroPostGoTextureFence
exit $LASTEXITCODE
