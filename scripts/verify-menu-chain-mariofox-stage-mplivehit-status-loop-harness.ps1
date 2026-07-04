param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$ImportBattleShipFTManager,
    [switch]$HardwareTriangles
)
$ErrorActionPreference = 'Stop'
$ImportBattleShipFTManager = $true
$target = if ($HardwareTriangles) { 'smash64ds-menu-chain-mariofox-stage-mplivehit-status-loop-hwtri' } else { 'smash64ds-menu-chain-mariofox-stage-mplivehit-status-loop' }
$build = if ($HardwareTriangles) { 'build-menu-chain-mariofox-stage-mplivehit-status-loop-hwtri-harness' } else { 'build-menu-chain-mariofox-stage-mplivehit-status-loop-harness' }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -ImportBattleShipFTManager:$ImportBattleShipFTManager `
    -HardwareTriangles:$HardwareTriangles `
    -Harness 'menu_chain_mariofox_stage_mplivehit_status_loop' `
    -Target $target `
    -Build $build `
    -ExpectedMode 162 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox Stage MP live-hit status-loop' `
    -HarnessSelectMessage 'Menu-chain Mario/Fox Stage MP live-hit status-loop harness did not select VS Mode from Title.'
