param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
)
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-process-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'menu_chain_mariofox_process_loop' `
    -Target 'smash64ds-menu-chain-mariofox-process-loop' `
    -Build 'build-menu-chain-mariofox-process-loop-harness' `
    -ExpectedMode 46 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox process-loop' `
    -HarnessSelectMessage 'Menu-chain process-loop harness did not start from VS Mode after Title.'
