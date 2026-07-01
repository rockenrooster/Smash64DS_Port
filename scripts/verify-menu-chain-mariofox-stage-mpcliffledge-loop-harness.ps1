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
    -Harness 'menu_chain_mariofox_stage_mpcliffledge_loop' `
    -Target 'smash64ds-menu-chain-mariofox-stage-mpcliffledge-loop' `
    -Build 'build-menu-chain-mariofox-stage-mpcliffledge-loop-harness' `
    -ExpectedMode 132 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox Stage MP cliff-ledge loop' `
    -HarnessSelectMessage 'Menu-chain Mario/Fox Stage MP cliff-ledge harness did not select VS Mode from Title.' `
    -RequireStageDraw `
    -RequireStageMPCliffLedgeLoop
exit $LASTEXITCODE
