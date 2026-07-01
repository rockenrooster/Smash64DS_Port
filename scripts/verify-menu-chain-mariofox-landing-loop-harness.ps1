param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
)
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-landing-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'menu_chain_mariofox_landing_loop' `
    -Target 'smash64ds-menu-chain-mariofox-landing-loop' `
    -Build 'build-menu-chain-mariofox-landing-loop-harness' `
    -ExpectedMode 44 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox Landing-loop' `
    -HarnessSelectMessage 'Menu-chain Landing-loop harness did not start from VS Mode after Title.'
