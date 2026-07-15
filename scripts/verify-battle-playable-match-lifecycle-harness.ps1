param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$RunnerSlot = -1,
    [int]$GdbPort = 4333,
    [switch]$NoBuild,
    [int]$DelaySeconds = 3
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')

$selectedGdbPort = if (($RunnerSlot -ge 0) -and
    -not $PSBoundParameters.ContainsKey('GdbPort')) {
    Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
} else {
    $GdbPort
}

& (Join-Path $PSScriptRoot 'verify-battle-playable-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -RunnerSlot $RunnerSlot `
    -GdbPort $selectedGdbPort `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -ImportBattleShipFTComputer `
    -CPUOpponentProof `
    -MatchLifecycleProof
exit $LASTEXITCODE
