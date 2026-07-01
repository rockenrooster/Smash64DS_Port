param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-controller-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'menu_chain_mariofox_controller_loop' `
    -Target 'smash64ds-menu-chain-mariofox-controller-loop' `
    -Build 'build-menu-chain-mariofox-controller-loop-harness' `
    -ExpectedMode 50 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox controller-loop' `
    -HarnessSelectMessage 'Menu-chain controller-loop harness did not start at VS Mode from Title.'
