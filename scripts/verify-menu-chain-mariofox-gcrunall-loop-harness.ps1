param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [switch]$ImportBattleShipFTManager,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
$ImportBattleShipFTManager = $true
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -ImportBattleShipFTManager:$ImportBattleShipFTManager `
    -DelaySeconds $DelaySeconds `
    -Harness 'menu_chain_mariofox_gcrunall_loop' `
    -Target 'smash64ds-menu-chain-mariofox-gcrunall-loop' `
    -Build 'build-menu-chain-mariofox-gcrunall-loop-harness' `
    -ExpectedMode 54 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox gcRunAll-loop' `
    -HarnessSelectMessage 'Menu-chain gcRunAll-loop harness did not start at VS Mode from Title.'
