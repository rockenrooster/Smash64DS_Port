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
    -Harness 'menu_chain_mariofox_stage_mpclifflive_loop' `
    -Target 'smash64ds-menu-chain-mariofox-stage-mpclifflive-loop' `
    -Build 'build-menu-chain-mariofox-stage-mpclifflive-loop-harness' `
    -ExpectedMode 134 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox Stage MP cliff-live loop' `
    -HarnessSelectMessage 'Menu-chain Mario/Fox Stage MP cliff-live harness did not select VS Mode from Title.' `
    -RequireStageDraw `
    -RequireStageMPCliffLiveLoop
exit $LASTEXITCODE
