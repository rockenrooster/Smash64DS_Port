param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'battle_mariofox_stage_mpcliffledge_loop' `
    -Target 'smash64ds-battle-mariofox-stage-mpcliffledge-loop' `
    -Build 'build-battle-mariofox-stage-mpcliffledge-loop-harness' `
    -ExpectedMode 131 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'Battle Mario/Fox Stage MP cliff-ledge loop' `
    -HarnessSelectMessage 'Direct Mario/Fox Stage MP cliff-ledge harness did not select VSBattle from Maps.' `
    -RequireStageDraw `
    -RequireStageMPCliffLedgeLoop
exit $LASTEXITCODE
