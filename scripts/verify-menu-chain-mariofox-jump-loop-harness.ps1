param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-jump-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'menu_chain_mariofox_jump_loop' `
    -Target 'smash64ds-menu-chain-mariofox-jump-loop' `
    -Build 'build-menu-chain-mariofox-jump-loop-harness' `
    -ExpectedMode 42 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox Jump-loop' `
    -HarnessSelectMessage 'Menu-chain Jump-loop harness did not select VS Mode from Title.'
