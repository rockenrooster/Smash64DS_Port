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
    -Harness 'menu_chain_mariofox_stage_mpceilstatus_floor_loop' `
    -Target 'smash64ds-menu-chain-mariofox-stage-mpceilstatus-floor-loop' `
    -Build 'build-menu-chain-mariofox-stage-mpceilstatus-floor-loop-harness' `
    -ExpectedMode 98 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox Stage MP Ceiling-status floor-loop' `
    -HarnessSelectMessage 'Menu-chain Mario/Fox Stage MP Ceiling-status floor-loop harness did not select VS Mode from Title.' `
    -RequireStageDraw `
    -RequireStageMPCeilStatusFloor
exit $LASTEXITCODE
