param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$ImportBattleShipFTManager
)
$ErrorActionPreference = 'Stop'
$ImportBattleShipFTManager = $true
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -ImportBattleShipFTManager:$ImportBattleShipFTManager `
    -Harness 'menu_chain_mariofox_dash_run' `
    -Target 'smash64ds-menu-chain-mariofox-dash-run' `
    -Build 'build-menu-chain-mariofox-dash-run-harness' `
    -ExpectedMode 40 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox Dash-Run' `
    -HarnessSelectMessage 'Menu-chain Dash-Run harness did not start at VS Mode from Title.'
